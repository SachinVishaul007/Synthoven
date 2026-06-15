package com.chopaudio.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.reactive.function.client.WebClient;

@Configuration
public class WebClientConfig {

    @Value("${chop.generation.suno.base-url}")
    private String sunoBaseUrl;

    @Value("${chop.generation.suno.api-key}")
    private String sunoApiKey;

    @Value("${chop.generation.stableaudio.base-url}")
    private String stableAudioBaseUrl;

    @Value("${chop.generation.stableaudio.api-key}")
    private String stableAudioApiKey;

    @Bean(name = "sunoWebClient")
    public WebClient sunoWebClient() {
        return WebClient.builder()
                .baseUrl(sunoBaseUrl)
                .defaultHeader("Authorization", "Bearer " + sunoApiKey)
                .defaultHeader("Content-Type", "application/json")
                .build();
    }

    @Bean(name = "stableAudioWebClient")
    public WebClient stableAudioWebClient() {
        // Stable Audio responds with the raw audio file in the body, which easily
        // exceeds WebClient's 256 KB default buffer — raise it to 64 MB.
        return WebClient.builder()
                .baseUrl(stableAudioBaseUrl)
                .defaultHeader("Authorization", "Bearer " + stableAudioApiKey)
                .codecs(c -> c.defaultCodecs().maxInMemorySize(64 * 1024 * 1024))
                .build();
    }
}