package com.kplayer.app;

final class RadioStation {
    final String name, country, codec, url, favicon;
    final int bitrate;
    RadioStation(String name, String country, String codec, String url, String favicon, int bitrate) {
        this.name = name; this.country = country; this.codec = codec; this.url = url;
        this.favicon = favicon; this.bitrate = bitrate;
    }
}
