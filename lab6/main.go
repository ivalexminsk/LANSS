package main

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"net"
	"os"
	"os/signal"
)

const udpNet = "udp"
const allAddr = "0.0.0.0"

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

	switch c.Mode {
	case "multicast":
		log.Fatal("Not implemented yet")
	case "broadcast":
		runBroadcast(c.Port)
	default:
		log.Fatal("Unsupported mode", c.Mode)
	}
}

func runBroadcast(port int) {
	udpaddr := net.UDPAddr{
		IP:   net.ParseIP(allAddr),
		Port: port,
	}
	conn, err := net.ListenUDP(udpNet, &udpaddr)
	if err != nil {
		log.Fatal(err)
	}

	defer conn.Close()

	buff := make([]byte, 1024)
	for {
		n, src, err := conn.ReadFrom(buff)
		if err != nil {
			log.Fatal(err)
		}

		log.Printf("From %v: %s", src, string(buff[:n]))
	}
}
