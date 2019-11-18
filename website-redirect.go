package main

import (
	"log"
	"net/http"
	"net/url"

	"github.com/esote/openshim2"
)

func init() {
	if err := openshim2.LazySysctls(); err != nil {
		log.Fatal(err)
	}
}

func redirect(w http.ResponseWriter, r *http.Request) {
	host, err := url.Parse("http://" + r.Host)

	if err != nil || host.Hostname() == "" {
		return
	}

	u := &url.URL{
		Scheme: "https",
		Host:   host.Hostname() + ":443",
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
