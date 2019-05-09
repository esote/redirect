// Copyright 2018 Esote. All rights reserved. Use of this source code is
// governed by an MIT license that can be found in the LICENSE file.

// Redirects from one port to another.

package main

import (
	"log"
	"net"
	"net/http"
	"net/url"
	"os"
	"syscall"

	"github.com/esote/graceful"
	"golang.org/x/sys/unix"
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
	initFlags()

	if err := syscall.Chroot("."); err != nil {
		log.Fatal(err)
	}

	// force init of lazy sysctls
	if l, err := net.Listen("tcp", "localhost:0"); err != nil {
		log.Fatal(err)
	} else {
		l.Close()
	}

	if err := unix.Pledge("stdio inet", ""); err != nil {
		log.Fatal(err)
	}

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
