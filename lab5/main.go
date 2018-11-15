package main

import (
	"encoding/binary"
	"encoding/json"
	"errors"
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
const traceMaxWaitPeriod = time.Second * 5
const switchBuffSize = 5
const beginPingSeqValue = 1
const ttlErrorValue = -1
const traceMaxHops = 30
const ethMaxFrameLen = 1500

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
	ttl         int
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
		} else {
			log.Println("Unsupported channel ID. Message: ", ri)
		}
	})
}

func runSwitching(p *ipv4.PacketConn, appNeedClose chan os.Signal, cb func(id int, ri routineInfo)) {
	rb := make([]byte, ethMaxFrameLen)

	// emable all (simplier)
	err := p.SetControlMessage(ipv4.FlagTTL|ipv4.FlagSrc|ipv4.FlagDst|ipv4.FlagInterface, true)
	if err != nil {
		log.Println("Cannot enable ttl feature: ", err)
	}

	for {
		select {
		case <-appNeedClose:
			// close all
			return
		default:
		}

		n, cm, src, err := p.ReadFrom(rb)
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

		if cm != nil {
			toSend.ttl = cm.TTL
		} else {
			toSend.ttl = ttlErrorValue
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

				fmt.Printf("Response from %v: seq_id=%d, time=%v, ttl=%v\n",
					ans.src, ans.icmpMessage.Body.(*icmp.Echo).Seq,
					t, ans.ttl)
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
			msg := makePingMessage(id, dataSuffix, transmitted)

			if _, err := conn.Write(msg); err != nil {
				log.Fatal(err)
			}
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

func makePingMessage(id []byte, dataSuffix []byte, seqID int) (msg []byte) {
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

	msg, err := wm.Marshal(nil)
	if err != nil {
		log.Fatal(err)
	}

	return
}

func messageSwitchParsing(rb []byte, src net.Addr) (ri routineInfo, reqID int, ok bool) {
	ok = true

	rm, err := icmp.ParseMessage(icmpID, rb)
	if err != nil {
		log.Fatal(err)
	}

	requiredIDData, err := getICMPData(rm)
	if err != nil {
		log.Fatal(err)
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

func getICMPData(rm *icmp.Message) (icmpData []byte, err error) {

	switch rm.Type {
	case ipv4.ICMPTypeEcho:
		fallthrough
	case ipv4.ICMPTypeEchoReply:
		icmpData = rm.Body.(*icmp.Echo).Data
	case ipv4.ICMPTypeDestinationUnreachable:
		icmpData = rm.Body.(*icmp.DstUnreach).Data

		icmpData, err = parseIPPacketToICMPData(icmpData)

	case ipv4.ICMPTypeTimeExceeded:
		icmpData = rm.Body.(*icmp.TimeExceeded).Data
	default:
		err = errors.New("Unsupported ICMP packet type")
	}
	return
}

func parseIPPacketToICMPData(ipPacket []byte) (icmpData []byte, err error) {
	h, err := ipv4.ParseHeader(ipPacket)
	if err != nil {
		return
	}
	if h.Protocol != icmpID {
		err = errors.New("Not ICMP protocol")
	}

	rm, err := icmp.ParseMessage(icmpID, ipPacket[h.Len:])
	if err != nil {
		return
	}
	icmpData, err = getICMPData(rm)

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

	dataSuffix := []byte("HELLO-FROM-IvAlex-Minsk-traceroute")

	dstNet, ok := resolveIP(addr)
	if !ok {
		log.Fatal("Cannot resolve address", addr, "\n")
	}

	fmt.Printf("Traceroute %v (%s)\n", dstNet.IP, addr)

	rb := make([]byte, ethMaxFrameLen)
	// ticker := time.NewTicker(tracePeriod)
	// var rtt []time.Duration

	for i := 1; i <= traceMaxHops; i++ {
		select {
		case <-appNeedClose:
			return
		default:
			//send trace message
			traceID := make([]byte, idLenBytes)
			binary.LittleEndian.PutUint32(traceID, uint32(i))

			err := p.SetTTL(i)
			if err != nil {
				log.Fatal(err)
			}

			msg := makePingMessage(traceID, dataSuffix, i)

			_, err = p.WriteTo(msg, nil, &dstNet)
			if err != nil {
				log.Fatal(err)
			}

			p.SetDeadline(time.Now().Add(traceMaxWaitPeriod))

			n, _, src, err := p.ReadFrom(rb)
			if err != nil {
				if err, ok := err.(net.Error); ok && err.Timeout() {
					printTraceInfoTimeout(i)
					continue
				}
				log.Fatal(err)
			}

			rbCurr := rb[:n]
			ans, id, ok := messageSwitchParsing(rbCurr, src)
			if !ok {
				log.Println("Error: cannot parse packet:", rbCurr)
				continue
			}

			t := time.Since(ans.time)

			switch ans.icmpMessage.Type {
			case ipv4.ICMPTypeEchoReply:
				printTraceInfo(id, ans.src, t)
				fmt.Printf("\nCompleted: success\n")
				return
			case ipv4.ICMPTypeDestinationUnreachable:
				printTraceInfo(id, ans.src, t)
				fmt.Printf("\nCompleted: destination host unreachable\n")
				return
			case ipv4.ICMPTypeTimeExceeded:
				printTraceInfo(id, ans.src, t)
			default:
				log.Printf("Unsupported message type %v when host %v is traced\n",
					ans.icmpMessage.Type, addr)
			}
		}
	}

	log.Printf("Cannot reach host %v (%v) with %d hops\n",
		dstNet.IP, addr, traceMaxHops)
}

func resolveIP(addr string) (dst net.IPAddr, ok bool) {
	ip := net.ParseIP(addr)
	if ip != nil {
		dst.IP = ip
		ok = true
		return
	}

	ips, err := net.LookupIP(addr)
	if err != nil {
		log.Fatal(err)
	}
	for _, ip := range ips {
		if ip.To4() != nil {
			dst.IP = ip
			break
		}
	}
	if dst.IP != nil {
		ok = true
	}

	return
}

func printTraceInfo(id int, src net.Addr, delta time.Duration) {
	fmt.Printf("%d.\t%v\t%v\n", id, delta, src)
}

func printTraceInfoTimeout(seqID int) {
	fmt.Printf("%d.\t*\tTimeout\n", seqID)
}
