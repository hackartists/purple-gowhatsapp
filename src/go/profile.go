package main

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
)

type ProfilePictureRequest struct {
	who          string
	picture_date string
}

func (handler *Handler) request_profile_picture(who string, picture_date string) {
	handler.pictureRequests <- ProfilePictureRequest{who: who, picture_date: picture_date}
}

func (handler *Handler) profile_picture_downloader() {
	if handler.httpClient != nil {
		return
	}
	handler.httpClient = &http.Client{}
	log := handler.log.Sub("Profile")
	emptyRequest := ProfilePictureRequest{}
	for pdr := range handler.pictureRequests {
		if pdr == emptyRequest {
			log.Infof("WhatsApp session disconnected. Profile picture downloader is shutting down.")
			return
		}
		if handler.httpClient == nil {
			log.Infof("Profile picture downloader has been removed.")
			return
		}
		if handler.client == nil && !handler.client.IsConnected() {
			// drop requests while not connected to WhatsApp
			continue
		}
		jid, err := parseJID(pdr.who)
		if err != nil {
			purple_error(handler.account, fmt.Sprintf("%#v", err), ERROR_FATAL)
			continue
		}
		preview := true
		ppi, _ := handler.client.GetProfilePictureInfo(jid, preview)
		if ppi == nil {
			// no picture set
			continue
		}
		req, err := http.NewRequest("GET", ppi.URL, nil)
		if pdr.picture_date != "" {
			req.Header.Add("If-Modified-Since", pdr.picture_date)
		}
		resp, err := handler.httpClient.Do(req)
		if err != nil {
			log.Warnf("Error downloading profile picture for %s: %#v", pdr.who, err)
			continue
		}
		defer resp.Body.Close()
		if resp.StatusCode == 304 {
			continue
		}
		var b bytes.Buffer
		_, err = io.Copy(&b, resp.Body)
		if err != nil {
			log.Warnf("Error while transferring profile picture for %s: %#v", pdr.who, err)
			continue
		}
		purple_set_profile_picture(handler.account, pdr.who, b.Bytes(), resp.Header.Get("Last-Modified"))
	}
}