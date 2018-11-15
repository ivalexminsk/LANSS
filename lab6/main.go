package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"os"
	"os/signal"
	"sync"
	"time"
)

const udpNet = "udp"
const allAddr = "0.0.0.0"
const recvBuff = 2048
const recvTimeout = time.Second * 1
const consoleReadLineBuffSize = 3

const (
	protoConn    byte = 'c'
	protoDisconn byte = 'd'
	protoMessage byte = 'm'
)

func main() {
	type Config struct {
		Mode          string `json:"mode"`
		MulticastAddr string `json:"multicast_addr"`
		Port          int    `json:"net_port"`
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

	appNeedClose := make(chan os.Signal, 1)
	signal.Notify(appNeedClose, os.Interrupt)

	fmt.Println("Press Ctrl+C to exit")

	switch c.Mode {
	case "multicast":
		log.Fatal("Not implemented yet")
	case "broadcast":
		runBroadcast(c.Port, appNeedClose)
	default:
		log.Fatal("Unsupported mode ", c.Mode)
	}

	fmt.Println("Bye")
}

func runBroadcast(port int, exitDetect chan os.Signal) {
	fmt.Println("Broadcast mode was selected. Working on port", port)

	udpaddr := net.UDPAddr{
		IP:   net.ParseIP(allAddr),
		Port: port,
	}
	conn, err := net.ListenUDP(udpNet, &udpaddr)
	if err != nil {
		log.Fatal(err)
	}

	defer conn.Close()

	var wg sync.WaitGroup
	wg.Add(2)

	recvChan := make(chan bool, 1)
	userWaitChan := make(chan bool, 1)

	go asyncRecvEcho(conn, recvChan, &wg)
	go asyncUserInput(conn, userWaitChan, &wg)

	//wait for end
	<-exitDetect

	//need to stop
	recvChan <- true
	userWaitChan <- true

	wg.Wait()
}

func asyncRecvEcho(conn *net.UDPConn, exitDetect chan bool, wg *sync.WaitGroup) {
	defer wg.Done()

	buff := make([]byte, recvBuff)

	sendConn(conn)
	defer sendDisconn(conn)

	for {
		select {
		case <-exitDetect:
			return
		default:
			err := conn.SetReadDeadline(time.Now().Add(recvTimeout))
			if err != nil {
				log.Fatal(err)
			}

			n, src, err := conn.ReadFrom(buff)
			if err != nil {
				if e, ok := err.(net.Error); ok && e.Timeout() {
					//it's a timeout
					break
				}

				log.Fatal(err)
			}

			parsePrintRecv(buff[:n], src)
		}
	}
}

func asyncUserInput(conn *net.UDPConn, exitDetect chan bool, wg *sync.WaitGroup) {
	defer wg.Done()

	readChan := make(chan string, consoleReadLineBuffSize)
	go asyncConsoleRead(readChan)

	for {
		select {
		case <-exitDetect:
			return
		case s := <-readChan:
			sendMessage(conn, s)
		}
	}
}

func asyncConsoleRead(readInfo chan string) {
	var str string
	for {
		fmt.Scanln(str)
		readInfo <- str
	}
}

func sendRaw(conn *net.UDPConn, bs []byte) {
	// conn.WriteTo()
	//TODO:
}

func sendConn(conn *net.UDPConn) {
	sendRaw(conn, []byte{protoConn})
}

func sendDisconn(conn *net.UDPConn) {
	sendRaw(conn, []byte{protoDisconn})
}

func sendMessage(conn *net.UDPConn, mess string) {
	toSend := []byte{protoMessage}
	toSend = append(toSend, []byte(mess)...)
	sendRaw(conn, toSend)
}

func parsePrintRecv(recv []byte, src net.Addr) {
	if len(recv) < 1 {
		return
	}

	control := recv[0]
	switch control {
	case protoConn:
		log.Println("Connected:", src)
	case protoDisconn:
		log.Println("Disconnected:", src)
	case protoMessage:
		log.Printf("From %v: %s", src, string(recv[1:]))
	default:
		log.Println("Unknown packet key:", control)
	}
}
