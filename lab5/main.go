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
	conn, err := net.Dial("ip4:icmp", addr)
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
	n, err := conn.Read(rb)
	if err != nil {
		log.Fatal(err)
	}

	for _, element := range rb[:n] {
		// element is the element from someSlice for where we are
		fmt.Printf("%02X ", element)
	}

	ipv4Header, err := icmp.ParseIPv4Header(rb[:n])
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("\nrmm: %v", ipv4Header)
	fmt.Printf("\nrmm: %T", ipv4Header)

	rm, err := icmp.ParseMessage(1, rb[:n]) //1 for ICMP v4
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
