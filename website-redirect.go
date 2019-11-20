package main

import (
	"log"
	"net/http"
	"net/url"
	"strings"

	"github.com/esote/openshim2"
)

func init() {
	if err := openshim2.LazySysctls(); err != nil {
		log.Fatal(err)
	}
}

func redirect(w http.ResponseWriter, r *http.Request) {
	host, err := url.Parse("http://" + r.Host)
	if err != nil {
		return
	}

	hostname := host.Hostname()
	if hostname == "" {
		return
	}

	/* Hostname will be localhost, since it comes from the reverse proxy. */
	i := strings.LastIndex(hostname, "localhost")
	if i == -1 {
		return
	}

	end := strings.Replace(hostname[i:], "localhost", "esote.net", 1)

	u := &url.URL{
		Scheme: "https",
		Host:   hostname[:i] + end + ":443",
	}

	http.Redirect(w, r, u.String(), http.StatusPermanentRedirect)
}

func main() {
	if err := openshim2.Unveil("/", ""); err != nil {
		log.Fatal(err)
	}
	if err := openshim2.Pledge("stdio inet", ""); err != nil {
		log.Fatal(err)
	}

	srv := &http.Server{
		Addr:    ":8441",
		Handler: http.HandlerFunc(redirect),
	}

	if err := srv.ListenAndServe(); err != nil {
		log.Fatal(err)
	}
}
