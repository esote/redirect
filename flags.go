// Copyright 2018 Esote. All rights reserved. Use of this source code is
// governed by an MIT license that can be found in the LICENSE file.

package main

import "flag"

type flags struct {
	sourcePort string
	targetPort string

	sourceScheme string
	targetScheme string
}

var fl flags

func initFlags() {
	dfl := flags{
		sourcePort: "80",
		targetPort: "443",

		sourceScheme: "http",
		targetScheme: "https",
	}

	flag.StringVar(&fl.sourcePort, "S", dfl.sourcePort, "source port")
	flag.StringVar(&fl.targetPort, "T", dfl.targetPort, "target port")

	flag.StringVar(&fl.sourceScheme, "s", dfl.sourceScheme, "source scheme")
	flag.StringVar(&fl.targetScheme, "t", dfl.targetScheme, "target scheme")

	flag.Parse()

	if fl.sourcePort[0] != ':' {
		fl.sourcePort = ":" + fl.sourcePort
	}

	if fl.targetPort[0] != ':' {
		fl.targetPort = ":" + fl.targetPort
	}
}
