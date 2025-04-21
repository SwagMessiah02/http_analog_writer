#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"

#define HOST "server-y824.onrender.com"
#define URL_REQUEST "/mensagem?msg="     
#define BTB 6
#define BUFFER_SIZE 512

struct Pos {
    uint x_pos;
    uint y_pos;
};

// Cabeçalho das funções utilizadas 
void urlencode(const char *input, char *output, size_t output_size);
void button_callback(uint gpio, uint32_t event_mask);
const char* posicao(struct Pos pos);
void set_pos(struct Pos *pos);
void lerPosicao();
void enviarDados(const char* data);
void enviarTemperatura();
void setup();

int main() {
    setup();

    // Inicia a comunicação via wi-fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return 1;
    }

    // Habilita o modo estação wi-fi
    cyw43_arch_enable_sta_mode();

    // Tenta iniciar uma conexão com uma rede wi-fi
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha na conexão Wi-Fi\n");
        return 1;
    }

    sleep_ms(1000);

    printf("Wi-Fi conectado!\n");

    while (true) {
        struct Pos pos;
        set_pos(&pos);
        lerPosicao(pos);
        
        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}

// Atualiza a posição das cordenadas x e y de acordo com a posição do joystick
void set_pos(struct Pos *pos) {
    adc_select_input(1);
    uint16_t adc_x_raw = adc_read();

    adc_select_input(0);
    uint16_t adc_y_raw = adc_read();

    const uint bar_width = 40;
    const uint adc_max = (1 << 12) - 1; 
    uint bar_x_pos = adc_x_raw * bar_width / adc_max;
    uint bar_y_pos = adc_y_raw * bar_width / adc_max;

    pos->x_pos = bar_x_pos;
    pos->y_pos = bar_y_pos;
}

void urlencode(const char *input, char *output, size_t output_size) {
    char hex[] = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; input[i] && j + 3 < output_size; i++) {
        if ((input[i] >= 'A' && input[i] <= 'Z') || 
            (input[i] >= 'a' && input[i] <= 'z') || 
            (input[i] >= '0' && input[i] <= '9') || 
            input[i] == '-' || input[i] == '_' || input[i] == '.' || input[i] == '~') {
            output[j++] = input[i];
        } else {
            output[j++] = '%';
            output[j++] = hex[(input[i] >> 4) & 0xF];
            output[j++] = hex[input[i] & 0xF];
        }
    }
    output[j] = '\0';
}

// Função que envia os dados para o servidor
void enviarDados(const char* data) {
    char encoded_data[BUFFER_SIZE];
    urlencode(data, encoded_data, BUFFER_SIZE);

    char full_url[BUFFER_SIZE];
    snprintf(full_url, BUFFER_SIZE, "%s%s", URL_REQUEST, encoded_data);

    EXAMPLE_HTTP_REQUEST_T req = {0};
    req.hostname = HOST;
    req.url = full_url;
    req.tls_config = altcp_tls_create_config_client(NULL, 0);
    req.headers_fn = http_client_header_print_fn;
    req.recv_fn = http_client_receive_print_fn;

    printf("Enviando dados: %s\n", data);
    int result = http_client_request_sync(cyw43_arch_async_context(), &req);

    altcp_tls_free_config(req.tls_config);
}

// Inicializa os periféricos do microcontrolador
void setup() {
    stdio_init_all();
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    gpio_init(BTB);
    gpio_set_dir(BTB, GPIO_IN);
    gpio_pull_up(BTB); 
}

// Função para ler a temperatura 
void lerPosicao(struct Pos pos) {
    // Armazena no buffer o conteúdo a ser enviado ao servido 
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[ X: %d   Y: %d ] | %s\n", pos.x_pos, pos.y_pos, posicao(pos));

    enviarDados(buffer);
}

// Retrona a orientação do joystick na rosa dos ventos
const char* posicao(struct Pos pos) {
    if(pos.x_pos == 19 && (pos.y_pos > 19 && pos.y_pos <= 39)) {
        return "Norte";
    } else if(pos.x_pos == 19 && (pos.y_pos < 19 && pos.y_pos >= 0)) {
        return "Sul";
    } else if((pos.x_pos < 19 && pos.x_pos >= 0) && pos.y_pos == 19) {
        return "Oeste";
    } else if((pos.x_pos > 19 && pos.x_pos <= 39) && pos.y_pos == 19) {
        return "Leste";
    } else if((pos.x_pos > 19 && pos.x_pos <= 39) && (pos.y_pos < 19 && pos.y_pos >= 0)) {
        return "Sudeste";
    } else if((pos.x_pos > 19 && pos.x_pos <= 39) && (pos.y_pos > 19 && pos.y_pos <= 39)) {
        return "Nordeste";
    } else if((pos.x_pos < 19 && pos.x_pos >= 0) && (pos.y_pos > 19 && pos.y_pos <= 39)) {
        return "Noroeste";
    } else if((pos.x_pos < 19 && pos.x_pos >= 0) && (pos.y_pos < 19 && pos.y_pos >= 0)) {
        return "Sudoeste";
    } else {
        return " ";
    }
} 