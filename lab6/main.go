package main

import (
	"bufio"
	"bytes"
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
const inputCommandDelim = ' '
const inputEndlDelim = '\n'
const detectOtherTimeout = time.Second * 1

const (
	protoConn      byte = 'c'
	protoDisconn   byte = 'd'
	protoMessage   byte = 'm'
	protoEcho      byte = 'e'
	protoEchoReply byte = 'r'
)

const (
	userMessage            string = "send"
	userDetectOtherClients string = "other"
)

//interface to work with
var sendIf ifInfo

func main() {
	type Config struct {
		Mode          string `json:"mode"`
		MulticastAddr string `json:"multicast_addr"`
		Port          int    `json:"net_port"`
		HwMac         string `json:"hw_mac"`
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

	sendIf, err = getIntByHwMac(c.HwMac)
	if err != nil {
		log.Fatal(err)
	}

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
	sendIf.sendIP = getIPBroadcast(sendIf.ip, sendIf.mask)
	sendIf.sendPort = port

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

			parsePrintRecv(buff[:n], src, conn)
		}
	}
}

type inputInfo struct {
	command string
	params  string
}

func asyncUserInput(conn *net.UDPConn, exitDetect chan bool, wg *sync.WaitGroup) {
	defer wg.Done()

	readChan := make(chan inputInfo, consoleReadLineBuffSize)
	go asyncConsoleRead(readChan)

	for {
		select {
		case <-exitDetect:
			return
		case info := <-readChan:
			switch info.command {
			case userMessage:
				sendMessage(conn, info.params)
			case userDetectOtherClients:
				detectOtherClients(conn)
			default:
				fmt.Printf("Command '%s' is not supported yet(\n", info.command)
			}
		}
	}
}

func asyncConsoleRead(readInfo chan inputInfo) {
	reader := bufio.NewReader(os.Stdin)
	for {
		var c string
		_, err := fmt.Scan(&c)
		if err != nil {
			log.Fatal(err)
		}

		str, err := reader.ReadString(inputEndlDelim)
		if err != nil {
			log.Fatal(err)
		}

		readInfo <- inputInfo{
			command: c,
			params:  str,
		}
	}
}

type ifInfo struct {
	ip       net.IP
	mask     net.IPMask
	iface    net.Interface
	sendIP   net.IP
	sendPort int
}

func sendRaw(conn *net.UDPConn, bs []byte) {
	udpaddr := net.UDPAddr{
		IP:   sendIf.sendIP,
		Port: sendIf.sendPort,
	}

	conn.WriteToUDP(bs, &udpaddr)
}

func sendConn(conn *net.UDPConn) {
	sendRaw(conn, []byte{protoConn})
}

func sendDisconn(conn *net.UDPConn) {
	sendRaw(conn, []byte{protoDisconn})
}

func sendEchoReply(conn *net.UDPConn) {
	sendRaw(conn, []byte{protoEchoReply})
}

func sendMessage(conn *net.UDPConn, mess string) {
	toSend := []byte{protoMessage}
	toSend = append(toSend, []byte(mess)...)
	sendRaw(conn, toSend)
}

type client struct {
	ip net.Addr
}

var currentClients []client

func detectOtherClients(conn *net.UDPConn) {
	//clear prev clients
	currentClients = make([]client, 0)

	sendRaw(conn, []byte{protoEcho})

	time.Sleep(detectOtherTimeout)

	fmt.Println("Other clients:")
	for _, c := range currentClients {
		fmt.Println(c.ip)
	}
}

func parsePrintRecv(recv []byte, src net.Addr, conn *net.UDPConn) {
	if len(recv) < 1 {
		return
	}

	if isIPEqual(sendIf.ip, src) {
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
	case protoEcho:
		sendEchoReply(conn)
	case protoEchoReply:
		currentClients = append(currentClients, client{
			ip: src,
		})
	default:
		log.Println("Unknown packet key:", control)
	}
}

func getAllInterfaceIPs(cb func(ii ifInfo)) {
	ifaces, err := net.Interfaces()
	if err != nil {
		log.Fatal(err)
	}
	for _, i := range ifaces {
		addrs, err := i.Addrs()
		if err != nil {
			log.Println(err)
			continue
		}
		for _, a := range addrs {
			switch v := a.(type) {
			case *net.IPNet:
				//only that address type supported
				ii := ifInfo{
					ip:    v.IP,
					mask:  v.Mask,
					iface: i,
				}

				cb(ii)
			}

		}
	}
}

func getIPBroadcast(ip net.IP, mask net.IPMask) net.IP {
	n := len(ip)
	if n != len(mask) {
		return nil
	}
	out := make(net.IP, n)
	for i := 0; i < n; i++ {
		out[i] = ip[i] | (^(mask[i]))
	}
	return out
}

func getIntByHwMac(hwMac string) (ii ifInfo, err error) {
	err = nil

	mac, err := net.ParseMAC(hwMac)
	if err != nil {
		return
	}

	ifaces, err := net.Interfaces()
	if err != nil {
		log.Fatal(err)
	}

iface_start:
	for _, i := range ifaces {
		// skip other interfaces
		if !bytes.Equal(i.HardwareAddr, mac) {
			continue
		}

		addrs, err := i.Addrs()
		if err != nil {
			log.Println(err)
			continue
		}

		for _, a := range addrs {
			switch v := a.(type) {
			case *net.IPNet:

				ip4 := v.IP.To4()
				if ip4 == nil {
					// skip ipv6 addrs
					break
				}

				// use first addr only
				ii = ifInfo{
					ip:    ip4,
					mask:  v.Mask,
					iface: i,
				}

				break iface_start
			}

		}
	}

	return
}

func isIPEqual(ip1 net.IP, ip2 net.Addr) bool {
	switch casted := ip2.(type) {
	case *net.UDPAddr:
		return ip1.Equal(casted.IP)
	}
	return false
}
