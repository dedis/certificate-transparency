package main

import (
	"encoding/json"
	"flag"
	"io/ioutil"
	"strings"

	"github.com/dedis/cothority/lib/app"
	"github.com/dedis/cothority/lib/cliutils"
	dbg "github.com/dedis/cothority/lib/debug_lvl"
	"github.com/dedis/crypto/abstract"
	"github.com/google/certificate-transparency/go/client"
	"github.com/dedis/cothority/lib/conode"
)

// The base url is http://ct.googleapis.com/aviator
var logUri = flag.String("log_uri", "http://localhost:8888", "CT log base URI")
var dump = flag.Bool("dump", false, "Dump request to uri")

// Our crypto-suite used in the program
var suite abstract.Suite

// The aggregate public key X0
var public abstract.Point

// the configuration file of the cothority tree used
var conf *app.ConfigConode


func main() {
	flag.Parse()

	dbg.Lvl2("loguri is ", *logUri)
	logClient := client.New(*logUri)
	STH, err := logClient.GetSTH()
	if err != nil {
		dbg.Fatal("Couldn't get STH:", err)
	}
	dbg.Lvlf2("STH is %+v", STH)

	if *dump {
		dbg.Lvl1("Dumping STH\n")
		ioutil.WriteFile("test_sth_cosi.json", []byte(STH.CosiSignature), 0660)
		ioutil.WriteFile("test_sth_sha256.json",
			[]byte(STH.SHA256RootHash.Base64String()), 0660)
	} else {
		ReadConf()

		reply, err := JSONtoSignature(STH.CosiSignature)
		if err != nil{
			dbg.Fatal("Couldn't convert CosiSignature", err)
		}
		dbg.Lvlf3("Reply is %+v", reply)
		dbg.Lvlf3("SHA256 of STH is %+v", STH.SHA256RootHash)
		if conode.VerifySignature(suite, reply, public, STH.SHA256RootHash[:]){
			dbg.Lvl1("Successfully received STH with hash", STH.SHA256RootHash[:])
			dbg.Lvl1("and checked conode-signature to be OK")
		} else {
			dbg.Lvl1("STH with hash", STH.SHA256RootHash, "didn't verify conode-signature.")
			dbg.Lvl1("Perhaps wrong config.toml?")
		}
	}
}

// Decodes the JSON coming from the CT-server and puts back in a 'StampReply'-structure
func JSONtoSignature(sigStr string) (*conode.StampReply, error) {
	tsm := &conode.TimeStampMessage{}
	err := json.Unmarshal([]byte(sigStr), &tsm)
	if err != nil {
		dbg.Lvl2("Couldn't unmarshal ", sigStr)
		return nil, err
	}

	return tsm.Srep, err
}

func SetSuite(suiteStr string) {
	suite = app.GetSuite(suiteStr)
}

func ReadConf() {
	conf = new(app.ConfigConode)
	err := app.ReadTomlConfig(conf, "config.toml")
	if err != nil {
		dbg.Fatal("Couldn't load config-file")
	}
	SetSuite(conf.Suite)
	public, _ = cliutils.ReadPub64(suite, strings.NewReader(conf.AggPubKey))
}
