package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"os"
	"os/signal"
	"sync"
	"time"

	"golang.org/x/net/icmp"
	"golang.org/x/net/ipv4"
)

const icmpID = 1
const icmpStringID = "ip4:icmp"
const idRoutineDelta = 234
const idLenBytes = 4
const int64Len = 8
const pingPeriod = time.Second * 1
const switchBuffSize = 5
const beginPingSeqValue = 1

func main() {
	type Config struct {
		Mode      string   `json:"mode"`
		Addresses []string `json:"addresses"`
		TraceAddr string   `json:"traceAddress"`
	}

	if len(os.Args) < 2 {
		log.Fatal("Set config file path in arg 1")
	}

	content, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		log.Println("Cannot read config file")
		log.Fatal(err)
	}

	c := Config{}
	json.Unmarshal(content, &c)

	// var isEnd = false
	appNeedClose := make(chan os.Signal, 1)
	signal.Notify(appNeedClose, os.Interrupt)

	switch c.Mode {
	case "ping":
		runPing(c.Addresses, appNeedClose)
		break
	case "traceroute":
		runTraceroute(c.TraceAddr, appNeedClose)
		break
	default:
		log.Fatal("Unsupported mode")
	}
}

type routineInfo struct {
	icmpMessage *icmp.Message
	src         net.Addr
	time        time.Time
}

func runPing(addr []string, appNeedClose chan os.Signal) {
	fmt.Println("Ping mode was selected")

	recvConn, err := net.ListenPacket(icmpStringID, "0.0.0.0") // ICMP for IPv4
	if err != nil {
		log.Fatal(err)
	}
	defer recvConn.Close()
	p := ipv4.NewPacketConn(recvConn)

	// goroutines creating
	var wg sync.WaitGroup
	wg.Add(len(addr))
	defer wg.Wait()

	channels := make([]chan routineInfo, len(addr))
	defer func() {
		for _, el := range channels {
			close(el)
		}
	}()

	for i, el := range addr {
		dataID := make([]byte, idLenBytes)
		binary.LittleEndian.PutUint32(dataID, uint32(i+idRoutineDelta))
		channels[i] = make(chan routineInfo, switchBuffSize)
		go pingThread(el, dataID, channels[i], &wg)
	}

	runSwitching(p, appNeedClose, func(id int, ri routineInfo) {
		if id < len(channels) {
			channels[id] <- ri
		}
	})
}

func runSwitching(p *ipv4.PacketConn, appNeedClose chan os.Signal, cb func(id int, ri routineInfo)) {
	rb := make([]byte, 1500)
	for {
		select {
		case <-appNeedClose:
			// close all
			return
		default:
		}

		n, _, src, err := p.ReadFrom(rb)
		if err != nil {
			if err, ok := err.(net.Error); ok && err.Timeout() {
				continue
			}
			log.Fatal(err)
		}

		rbReseived := rb[:n]

		toSend, requiredID, ok := messageSwitchParsing(rbReseived, src)
		if !ok {
			log.Println("Cannot parse message: ", rbReseived)
			continue
		}

		cb(requiredID, toSend)
	}
}

func pingThread(addr string, id []byte, channel chan routineInfo, wg *sync.WaitGroup) {
	defer wg.Done()

	conn, err := net.Dial(icmpStringID, addr)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	dataSuffix := []byte("HELLO-FROM-IvAlex-Minsk-ping")

	ticker := time.NewTicker(pingPeriod)
	transmitted := 0
	var rtt []time.Duration

	defer printStats(addr, &transmitted, &rtt)

	for {
		select {
		case ans, ok := <-channel:
			if !ok {
				return
			}

			switch ans.icmpMessage.Type {
			case ipv4.ICMPTypeEchoReply:
				t := time.Since(ans.time)
				rtt = append(rtt, t)

				fmt.Printf("Response from %v: seq_id=%d, time=%v\n",
					ans.src, ans.icmpMessage.Body.(*icmp.Echo).Seq,
					t)
			case ipv4.ICMPTypeDestinationUnreachable:
				fmt.Printf("Destination host %v unreachable (from %v)\n",
					addr, ans.src)
			case ipv4.ICMPTypeTimeExceeded:
				fmt.Printf("Host %v ping TTL = 0 (from %v)\n",
					addr, ans.src)
			default:
				log.Printf("Unsupported message type %v when host %v is pinged\n",
					ans.icmpMessage.Type, addr)
			}

		case <-ticker.C:
			transmitted++
			sendPingMessage(conn, id, dataSuffix, transmitted)
		}
	}
}

func printStats(addr string, transmitted *int, rtt *[]time.Duration) {
	// division by zero blocking
	if *transmitted == 0 {
		*transmitted = 1
	}

	received := len(*rtt)
	loss := 100 - (received * 100 / *transmitted)
	var min, max, avg time.Duration
	if received > 0 {
		min = (*rtt)[0]
		max = min

		for _, e := range *rtt {
			if min > e {
				min = e
			}
			if max < e {
				max = e
			}

			avg += e
		}

		avg /= time.Duration(len(*rtt))
	}

	fmt.Printf("%v statistics: %d transmitted, %d received, %d%% loss, rtt min/avg/max = %v/%v/%v\n",
		addr, *transmitted, received, loss, min, avg, max)
}

func sendPingMessage(conn net.Conn, id []byte, dataSuffix []byte, seqID int) {
	data := make([]byte, int64Len)
	nsec := time.Now().UnixNano()
	binary.LittleEndian.PutUint64(data, uint64(nsec))

	data = append(id, data...)
	data = append(data, dataSuffix...)

	wm := icmp.Message{
		Type: ipv4.ICMPTypeEcho,
		Code: 0,
		Body: &icmp.Echo{
			ID:   27, //not specified by standard
			Seq:  seqID,
			Data: data,
		},
	}

	wb, err := wm.Marshal(nil)
	if err != nil {
		log.Fatal(err)
	}
	if _, err := conn.Write(wb); err != nil {
		log.Fatal(err)
	}
}

func messageSwitchParsing(rb []byte, src net.Addr) (ri routineInfo, reqID int, ok bool) {
	ok = true

	rm, err := icmp.ParseMessage(icmpID, rb)
	if err != nil {
		log.Fatal(err)
	}

	var requiredIDData []byte
	switch rm.Type {
	case ipv4.ICMPTypeEchoReply:
		requiredIDData = rm.Body.(*icmp.Echo).Data
	case ipv4.ICMPTypeDestinationUnreachable:
		requiredIDData = rm.Body.(*icmp.DstUnreach).Data
	case ipv4.ICMPTypeTimeExceeded:
		requiredIDData = rm.Body.(*icmp.TimeExceeded).Data
	default:
		ok = false
		return
	}

	if len(requiredIDData) < (idLenBytes + 2*int64Len) {
		log.Println("Warning: unwaited data length")
		ok = false
		return
	}

	first := 0

	reqID = int(binary.LittleEndian.Uint32(
		requiredIDData[first:(first+idLenBytes)])) - idRoutineDelta
	first += idLenBytes

	timeNSec := int64(binary.LittleEndian.Uint64(
		requiredIDData[first:(first + int64Len)]))

	timestamp := time.Unix(0, timeNSec)

	ri = routineInfo{icmpMessage: rm, src: src, time: timestamp}

	return
}

func runTraceroute(addr string, appNeedClose chan os.Signal) {
	fmt.Println("Traceroute mode was selected")

	recvConn, err := net.ListenPacket(icmpStringID, "0.0.0.0") // ICMP for IPv4
	if err != nil {
		log.Fatal(err)
	}
	defer recvConn.Close()
	p := ipv4.NewPacketConn(recvConn)

	// goroutines creating
	var wg sync.WaitGroup
	wg.Add(len(addr))
	defer wg.Wait()

	channel := make(chan routineInfo, switchBuffSize)
	defer close(channel)

	dataID := make([]byte, idLenBytes)
	binary.LittleEndian.PutUint32(dataID, uint32(idRoutineDelta))
	go traceThread(addr, dataID, channel, &wg)

	runSwitching(p, appNeedClose, func(id int, ri routineInfo) {
		if id < 1 {
			channel <- ri
		}
	})
}

func traceThread(addr string, id []byte, channel chan routineInfo, wg *sync.WaitGroup) {
	defer wg.Done()

	conn, err := net.Dial(icmpStringID, addr)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	dataSuffix := []byte("HELLO-FROM-IvAlex-Minsk-traceroute")

	ticker := time.NewTicker(pingPeriod)
	transmitted := 0
	var rtt []time.Duration

	// defer printStats(addr, &transmitted, &rtt)

	for {
		select {
		case ans, ok := <-channel:
			if !ok {
				return
			}

			switch ans.icmpMessage.Type {
			case ipv4.ICMPTypeEchoReply:
				t := time.Since(ans.time)
				rtt = append(rtt, t)

				fmt.Printf("Response from %v: seq_id=%d, time=%v\n",
					ans.src, ans.icmpMessage.Body.(*icmp.Echo).Seq,
					t)
			case ipv4.ICMPTypeDestinationUnreachable:
				fmt.Printf("Destination host %v unreachable (from %v)\n",
					addr, ans.src)
			case ipv4.ICMPTypeTimeExceeded:
				fmt.Printf("Host %v ping TTL = 0 (from %v)\n",
					addr, ans.src)
			default:
				log.Printf("Unsupported message type %v when host %v is pinged\n",
					ans.icmpMessage.Type, addr)
			}

		case <-ticker.C:
			transmitted++

			sendPingMessage(conn, id, dataSuffix, transmitted)
		}
	}
}
