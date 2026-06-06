package com.chopaudio;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableAsync;

@SpringBootApplication
@EnableAsync
public class ChopApplication {
    public static void main(String[] args) {
        SpringApplication.run(ChopApplication.class, args);
    }
}