// Copyright 2018 Esote. All rights reserved. Use of this source code is
// governed by an MIT license that can be found in the LICENSE file.

// Redirects from one port to another.

package main

import (
	"log"
	"net/http"
	"net/url"
	"os"
	"syscall"

	"github.com/esote/graceful"
	"github.com/esote/openshim"
)

func redirect(w http.ResponseWriter, r *http.Request) {
	host, err := url.Parse(fl.sourceScheme + "://" + r.Host)

	if err != nil {
		log.Print(err)
		return
	} else if host.Hostname() == "" {
		log.Print("invalid hostname")
		return
	}

	u := &url.URL{
		Scheme: fl.targetScheme,
		Host:   host.Hostname() + fl.targetPort,
	}

	http.Redirect(w, r, u.String(), http.StatusPermanentRedirect)
}

func main() {
	if err := syscall.Chroot("."); err != nil {
		log.Fatal(err)
	}

	if _, err := openshim.Pledge("stdio inet", ""); err != nil {
		log.Fatal(err)
	}

	initFlags()

	srv := &http.Server{
		Addr:    fl.sourcePort,
		Handler: http.HandlerFunc(redirect),
	}

	graceful.Graceful(srv, func() {
		if err := srv.ListenAndServe(); err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}, os.Interrupt)
}
