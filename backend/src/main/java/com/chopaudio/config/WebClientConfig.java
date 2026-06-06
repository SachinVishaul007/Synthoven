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

    @Bean(name = "sunoWebClient")
    public WebClient sunoWebClient() {
        return WebClient.builder()
                .baseUrl(sunoBaseUrl)
                .defaultHeader("Authorization", "Bearer " + sunoApiKey)
                .defaultHeader("Content-Type", "application/json")
                .build();
    }
}