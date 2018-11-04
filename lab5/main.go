package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"os"

	"golang.org/x/net/icmp"
	"golang.org/x/net/ipv4"
)

const icmpID = 1
const icmpStringID = "ip4:icmp"

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

	switch c.Mode {
	case "ping":
		runPing(c.Addresses)
		break
	case "traceroute":
		runTraceroute(c.Addresses)
		break
	default:
		log.Fatal("Unsupported mode")
	}
}

func runPing(addr []string) {
	fmt.Println("Ping mode was selected")

	//TODO:

	pingThread("172.16.1.1", 1)

	log.Println("Not implemented yet")
}

func runTraceroute(addr []string) {
	log.Println("Not implemented yet")
	//TODO:
}

func pingThread(addr string, id int) {
	c, err := net.ListenPacket(icmpStringID, "0.0.0.0") // ICMP for IPv4
	if err != nil {
		log.Fatal(err)
	}
	defer c.Close()
	p := ipv4.NewPacketConn(c)

	conn, err := net.Dial(icmpStringID, addr)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	log.Printf("Local IP: %s", conn.LocalAddr().String())
	log.Printf("Remote IP: %s", conn.RemoteAddr().String())

	wm := icmp.Message{
		Type: ipv4.ICMPTypeEcho,
		Code: 0,
		Body: &icmp.Echo{
			ID:   id,
			Seq:  1,
			Data: []byte("HELLO-R-U-THERE"),
		},
	}

	wb, err := wm.Marshal(nil)
	if err != nil {
		log.Fatal(err)
	}
	if _, err := conn.Write(wb); err != nil {
		log.Fatal(err)
	}

	rb := make([]byte, 1500)
	n, _, _, err := p.ReadFrom(rb)
	if err != nil {
		if err, ok := err.(net.Error); ok && err.Timeout() {
		}
		log.Fatal(err)
	}

	for _, element := range rb[:n] {
		// element is the element from someSlice for where we are
		fmt.Printf("%02X ", element)
	}

	rm, err := icmp.ParseMessage(icmpID, rb[:n])
	if err != nil {
		log.Fatal(err)
	}
	switch rm.Type {
	case ipv4.ICMPTypeEchoReply:
		log.Printf("got reflection from %v", conn.RemoteAddr())
	default:
		log.Printf("got %+v; want echo reply", rm)
	}

	//TODO:
}
