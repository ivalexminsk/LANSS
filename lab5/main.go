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
const pingPeriod = time.Second * 1
const switchBuffSize = 5

func main() {
	type Config struct {
		Mode      string   `json:"mode"`
		Addresses []string `json:"addresses"`
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
		runTraceroute(c.Addresses, appNeedClose)
		break
	default:
		log.Fatal("Unsupported mode")
	}
}

type routineInfo struct {
	icmpMessage *icmp.Message
	src         net.Addr
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

	// swithing & waiting timeout
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

		rm, err := icmp.ParseMessage(icmpID, rb[:n])
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
		}

		requiredID := int(binary.LittleEndian.Uint32(
			requiredIDData[:idLenBytes])) - idRoutineDelta

		if requiredID < len(channels) {
			toSend := routineInfo{icmpMessage: rm, src: src}
			channels[requiredID] <- toSend
		}
	}
}

func runTraceroute(addr []string, appNeedClose chan os.Signal) {
	log.Println("Not implemented yet")
	//TODO:
}

func pingThread(addr string, id []byte, channel chan routineInfo, wg *sync.WaitGroup) {
	defer wg.Done()

	conn, err := net.Dial(icmpStringID, addr)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	data := append(id, []byte("HELLO-FROM-IvAlex-Minsk")...)

	ticker := time.NewTicker(pingPeriod)
	for {
		select {
		case ans, ok := <-channel:
			if !ok {
				log.Printf("Routine close (addr %s)\n", addr)
				return
			}

			switch ans.icmpMessage.Type {
			case ipv4.ICMPTypeEchoReply:
				log.Printf("got reflection from %v", ans.src)
			default:
				log.Printf("got %+v; want echo reply", ans.icmpMessage)
			}

		case <-ticker.C:
			wm := icmp.Message{
				Type: ipv4.ICMPTypeEcho,
				Code: 0,
				Body: &icmp.Echo{
					ID:   27, //not specified by standard
					Seq:  1,
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

		default:
		}
	}
	//TODO:
}
