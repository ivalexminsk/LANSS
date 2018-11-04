package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
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

	fmt.Println(c.Mode)
	fmt.Println(c.Addresses)
}
