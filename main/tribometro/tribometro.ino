/*
*serial comunica mas cnc nao executa
 */
#include "HX711.h"

#define HMI_RX 2
#define HMI_TX 3

#define LCELL_Y_DT 23
#define LCELL_Y_CK 25
#define LCELL_Z_DT 27
#define LCELL_Z_CK 29

extern const int PULSE_X;
extern const int PULSE_Y;
extern const int PULSE_Z;
extern const int DIR_X;
extern const int DIR_Y;
extern const int DIR_Z;

const uint8_t termination[] = {0xFF, 0xFF, 0xFF};

typedef enum {
  iniciar_linear,
  iniciar_rotativo,
  iniciar_manual,
  iniciar_riscamento,
  iniciar_fadiga,
  cancelar,
  zerar,
  pronto,
  no_event
} evento_t;

typedef enum {
  standby,
  linear_alternativo,
  rotativo,
  manual,
  riscamento,
  fadiga
} estado_t;

typedef struct{
    uint8_t normal = 0;
    uint8_t curso = 0;
    uint8_t distancia = 0;
    uint8_t velocidade = 0;
} dados_linear_t;

typedef struct{
    uint8_t normal_inicial = 0;
    uint8_t normal_final = 0;
    uint8_t curso = 0;
    uint8_t distancia = 0;
    uint8_t velocidade = 0;
} dados_riscamento_t;

typedef struct{
    uint8_t normal_inicial = 0;
    uint8_t normal_final = 0;
    uint8_t curso = 0;
    uint8_t distancia = 0;
    uint8_t intervalo = 0;
    uint8_t velocidade = 0;
} dados_fadiga_t;

typedef struct{
    uint8_t normal = 0;
    uint8_t diametro = 0;
    uint8_t distancia = 0;
    uint8_t velocidade = 0;
} dados_rotativo_t;

evento_t standby_f();
evento_t linear_alternativo_f();
evento_t rotativo_f();
evento_t manual_f();
evento_t riscamento_f();
evento_t fadiga_f();
evento_t zerar_f();

float erro_integral = 0, erro_anterior = 0, forca_z = 0, referencia_forca_z = 0, prd_controle = 100e-6, kp = 0.1, ki = 1;
void reiniciar_controle();
void lei_de_controle();

estado_t estado_anterior = standby, estado_atual = standby, proximo_estado;
dados_fadiga_t dados_fadiga;
dados_linear_t dados_linear;
dados_riscamento_t dados_riscamento;
dados_rotativo_t dados_rotativo;

long last_maquina_de_estados = 0;
long last_cnc = 0;
extern const int passos_por_mm;

long tempo_cnc = 100; // Tempo de atualização do cnc em microssegundos, a alteração deste valor afeta a velocidade.

char cmd_buffer[10];

void init_timer3(uint16_t periodo_us){
    // Modo Fast PWM (overflow quando o contador exige OCR3A -> controlar o periodo)
    TCCR3A = (1 << WGM31) | (1 << WGM30);
    TCCR3B = (1 << WGM33) | (1 << WGM32);
    
    TCCR3B |= (1 << CS31); // Prescaler para clkio/64, resolução de 4 microssegundos

    OCR3A = (uint16_t)(periodo_us/4); // Configura o periodo do timer

    TIMSK3 = (1 << TOIE3); // habilita a interrupção de overflow
}

void init_timer4(uint16_t periodo_us){
    // Modo Fast PWM (overflow quando o contador exige OCR4A -> controlar o periodo)
    TCCR4A = (1 << WGM41) | (1 << WGM40);
    TCCR4B = (1 << WGM43) | (1 << WGM42);

    TCCR4B |= (1 << CS42); // Prescaler para clkio/64, resolução de 4 microssegundos

    OCR4A = (uint16_t)(periodo_us/4); // Configura o periodo do timer

    TIMSK4 = (1 << TOIE4); // habilita a interrupção de overflow
}

void habilitar_timer3_ovf(){
    TIMSK3 = (1 << TOIE3); // habilita a interrupção de overflow
}

void habilitar_timer4_ovf(){
    TIMSK4 = (1 << TOIE4); // habilita a interrupção de overflow
}

void desabilitar_timer3_ovf(){
    TIMSK3 = 0;
}

void desabilitar_timer4_ovf(){
    TIMSK4 = 0;
}

HX711 loadcell_y, loadcell_z;

void setup() {
    // Inicialização das portas seriais
    pinMode(PULSE_X, OUTPUT);
    pinMode(PULSE_Y, OUTPUT);
    pinMode(PULSE_Z, OUTPUT);
    pinMode(DIR_X, OUTPUT);
    pinMode(DIR_Y, OUTPUT);
    pinMode(DIR_Z, OUTPUT);
    Serial.begin(9600);
    Serial1.begin(9600);

    init_timer3(periodo_atualizacao_cnc(50)); // velocidade inicial de 50 mm/s
    init_timer4(1e6*prd_controle);

    desabilitar_timer4_ovf();

    loadcell_y.begin(LCELL_Y_DT, LCELL_Y_CK);
    loadcell_z.begin(LCELL_Z_DT, LCELL_Z_CK);

    sei();
}

void loop() {
    maquina_de_estados();
}

ISR(TIMER3_OVF_vect){
    atualizar_cnc();
}

ISR(TIMER4_OVF_vect){
    lei_de_controle();
}

extern int x_atual, y_atual, z_atual; // Posição em x, y e z em pulsos
extern int x_real, y_real, z_real; // Posição em x, y e z em pulsos

void maquina_de_estados() {
    evento_t evento = no_event;
    proximo_estado = estado_atual;

    switch (estado_atual) { // Executa as funções de estado baseado no estado atual
        case standby:
            evento = standby_f();
            break;
        case linear_alternativo:
            evento = linear_alternativo_f();
            break;
        case rotativo:
            evento = rotativo_f();
            break;
        case manual:
            evento = manual_f();
            break;
        case riscamento:
            evento = riscamento_f();
            break;
        case fadiga:
            evento = fadiga_f();
            break;
        default:
            break;
    }

    switch(estado_atual) { // Faz as transições de estado baseado no estado atual e no evento emitido por ele
        case standby:
            switch(evento){
                case iniciar_linear:
                    proximo_estado = linear_alternativo;
                    break;
                case iniciar_fadiga:
                    proximo_estado = fadiga;
                    break;
                case iniciar_manual:
                    proximo_estado = manual;
                    break;
                case iniciar_riscamento:
                    proximo_estado = riscamento;
                    break;
                case iniciar_rotativo:
                    proximo_estado = rotativo;
                    break;
                case zerar:
                    Serial.println("zero");
                    x_atual = 0;
                    y_atual = 0;
                    z_atual = 0;
                    break;
                default:
                    break;
            }
            break;
        case linear_alternativo:
            switch(evento){
                case cancelar:
                    proximo_estado = standby;
                    break;
                case pronto:
                    proximo_estado = standby;
                    break;
                default:
                    break;
            }
            break;
        case rotativo:
            switch(evento){
                case cancelar:
                    proximo_estado = standby;
                    break;
                case pronto:
                    proximo_estado = standby;
                    break;
                default:
                    break;
            }
            break;
        case manual:
            switch(evento){
                case cancelar:
                    proximo_estado = standby;
                    break;
                case pronto:
                    proximo_estado = standby;
                    break;
                default:
                    break;
            }
            break;
        case riscamento:
            switch(evento){
                case cancelar:
                    proximo_estado = standby;
                    break;
                case pronto:
                    proximo_estado = standby;
                    break;
                default:
                    break;
            }
            break;
        case fadiga:
            switch(evento){
                case cancelar:
                    proximo_estado = standby;
                    break;
                case pronto:
                    proximo_estado = standby;
                    break;
                default:
                    break;
            }
            break;
        default:
            proximo_estado = standby;
            break;
    }
    
    // Serial.print((int)estado_atual);
    // Serial.print(" ");
    // Serial.print((int)evento);
    // Serial.print(" ");
    // Serial.println((int)proximo_estado);

    estado_anterior = estado_atual;
    estado_atual = proximo_estado;
}

evento_t standby_f(){
    if(estado_anterior != standby){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        Serial.println("Standby");
    }

    if(Serial1.available() > 0){ // Se houverem dados a serem lidos do HMI, ler os três primeiros, que serão equivalentes ao cabeçalho do comando e se forem válidos, ler os dados para cada comando
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "lin") == 0){ // Comando para iniciar o modo linear
                numero_de_lidos = Serial1.readBytes((char *)&dados_linear, 4);
                if(numero_de_lidos == 3){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_linear;
                }
            }else if(strcmp(cmd_buffer, "rot") == 0){ // Comando para iniciar o modo rotação
                numero_de_lidos = Serial1.readBytes((char *)&dados_rotativo, 4);
                if(numero_de_lidos == 3){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_rotativo;
                }
            }else if(strcmp(cmd_buffer, "ris") == 0){ // Comando para iniciar o modo riscamento
                numero_de_lidos = Serial1.readBytes((char *)&dados_riscamento, 5);
                if(numero_de_lidos == 4){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_riscamento;
                }
            }else if(strcmp(cmd_buffer, "fad") == 0){ // Comando para iniciar o modo fadiga
                numero_de_lidos = Serial1.readBytes((char *)&dados_fadiga, 6);
                if(numero_de_lidos == 5){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_fadiga;
                }
            }else if(strcmp(cmd_buffer, "man") == 0){ // Comando para iniciar o modo manual
                return iniciar_manual;
            }else if(strcmp(cmd_buffer, "zro") == 0){ // Comando zerar
                x_atual = 0;
                y_atual = 0;
                z_atual = 0;
                x_real = 0;
                y_real = 0;
                z_real = 0;
                loadcell_y.tare(10);
                loadcell_z.tare(10);
                return zerar;
            }
        }
    }

    return no_event; // caso não haja dados na porta serial ou ocorra algum erro, não há evento emitido e o sistema continua em standby
}

unsigned long tempo_inicial = 0, tempo_step;
unsigned int total = 5000;
float delta_tempo = 0;
const float step = 0.1;

evento_t linear_alternativo_f(){
    if(estado_anterior != linear_alternativo){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        Serial.println("Modo Linear");
        Serial.print("Forca Normal (N): ");
        Serial.println(dados_linear.normal);
        Serial.print("Curso (mm): ");
        Serial.println(dados_linear.curso);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_linear.distancia);
        Serial1.print("Load.j0.val=0");
        tempo_inicial = millis();
        tempo_step = millis();
    }

    if(Serial1.available() > 0){
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "ccl") == 0){
                Serial1.print("page Menu");
                Serial1.write(termination, 3);
                return cancelar;
            }
        }
    }

    if(millis() - tempo_step >= step){
        Serial1.print("Load.j0.val+=");
        Serial1.print((uint8_t)(step*100/total));
        Serial1.write(termination, 3);
        tempo_step = millis();
    }

    if(millis() - tempo_inicial >= total){
        Serial1.print("page Menu");
        Serial1.write(termination, 3);
        return pronto;
    }

    return no_event;
}

evento_t rotativo_f(){
    if(estado_anterior != rotativo){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        cli();
        Serial.println("Modo Rotativo");
        Serial.print("Forca Normal (N): ");
        Serial.println(dados_rotativo.normal);
        Serial.print("Diametro (mm): ");
        Serial.println(dados_rotativo.diametro);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_rotativo.distancia);
        Serial.print("Velocidade (mm/s): ");
        Serial.println(dados_rotativo.velocidade);
        Serial1.print("Load.j0.val=0");

        init_timer3(periodo_atualizacao_cnc(dados_rotativo.velocidade));

        go_to_mm(0, dados_rotativo.diametro/2.0);
        go_to_circular_mm(2*3.1415926535897, 0, 0, 50);

        referencia_forca_z = dados_rotativo.normal;
        reiniciar_controle();
        habilitar_timer4_ovf();        

        tempo_inicial = millis();
        tempo_step = millis();
        delta_tempo = 0;
        sei();
    }

    if(Serial1.available() > 0){
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "ccl") == 0){
                desabilitar_timer4_ovf();
                limpar_fila();
                go_to_z_mm(0);
                go_to_mm(0, 0);
                return cancelar;
            }
        }
    }

    if(millis() - tempo_step >= 1000*step){
        Serial.print(loadcell_y.get_value(5));
        Serial.print("; ");
        Serial.println(delta_tempo);

        delta_tempo += step;
        tempo_step = millis();
    }

    if(cnc_complete()){
        Serial.println("Rotação completa!");
        return pronto;
    }

    return no_event;
}

evento_t manual_f(){
    if(estado_anterior != manual){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        Serial.println("Modo manual");
    }
    
    if(Serial1.available() > 0){
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "ccl") == 0){
                Serial1.print("page Menu");
                Serial1.write(termination, 3);
                return cancelar;
            }else if(strcmp(cmd_buffer, "hom") == 0){
                Serial.println("Home");
                go_to(0, 0);
            }else if(strcmp(cmd_buffer, "x++") == 0){
                Serial.println("x++");
                adicionar_incremento(1, 0, 0, 1*passos_por_mm); // incremento de 1mm
            }else if(strcmp(cmd_buffer, "x--") == 0){
                Serial.println("x--");
                adicionar_incremento(-1, 0, 0, 1*passos_por_mm); // incremento de 1mm
            }else if(strcmp(cmd_buffer, "y++") == 0){
                Serial.println("y++");
                adicionar_incremento(0, 1, 0, 1*passos_por_mm); // incremento de 1mm
            }else if(strcmp(cmd_buffer, "y--") == 0){
                Serial.println("y--");
                adicionar_incremento(0, -1, 0, 1*passos_por_mm); // incremento de 1mm
            }else if(strcmp(cmd_buffer, "z++") == 0){
                Serial.println("z++");
                adicionar_incremento(0, 0, 1, 1*passos_por_mm); // incremento de 1mm
            }else if(strcmp(cmd_buffer, "z--") == 0){
                Serial.println("z--");
                adicionar_incremento(0, 0, -1, 1*passos_por_mm); // incremento de 1mm
            }
        }
    }
    return no_event;
}

evento_t riscamento_f(){
    if(estado_anterior != riscamento){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        Serial.println("Modo Riscamento");
        Serial.print("Forca Normal Inicial (N): ");
        Serial.println(dados_riscamento.normal_inicial);
        Serial.print("Forca Normal Final (N): ");
        Serial.println(dados_riscamento.normal_final);
        Serial.print("Curso (mm): ");
        Serial.println(dados_riscamento.curso);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_riscamento.distancia);
        Serial1.print("Load.j0.val=0");
        Serial1.write(termination, 3);
        tempo_inicial = millis();
        tempo_step = millis();
    }

    if(Serial1.available() > 0){
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "ccl") == 0){
                return cancelar;
            }
        }
    }

    if(millis() - tempo_step >= step){
        Serial1.print("Load.j0.val+=");
        Serial1.print((uint8_t)(step*100/total));
        Serial1.write(termination, 3);
        tempo_step = millis();
    }

    if(millis() - tempo_inicial >= total){
        Serial1.print("page Menu");
        Serial1.write(termination, 3);
        return pronto;
    }

    return no_event;
}

evento_t fadiga_f(){
    if(estado_anterior != fadiga){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        Serial.println("Modo Fadiga");
        Serial.print("Forca Normal Inicial (N): ");
        Serial.println(dados_fadiga.normal_inicial);
        Serial.print("Forca Normal Final (N): ");
        Serial.println(dados_fadiga.normal_final);
        Serial.print("Curso (mm): ");
        Serial.println(dados_fadiga.curso);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_fadiga.distancia);
        Serial.print("Intervalo Entre Forcas (s): ");
        Serial.println(dados_fadiga.intervalo);
        Serial1.print("Load.j0.val=0");
        tempo_inicial = millis();
        tempo_step = millis();
    }

    if(Serial1.available() > 0){
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "ccl") == 0){
                return cancelar;
            }
        }
    }

    if(millis() - tempo_step >= step){
        Serial1.print("Load.j0.val+=");
        Serial1.print((uint8_t)(step*100/total));
        Serial1.write(termination, 3);
        tempo_step = millis();
    }

    if(millis() - tempo_inicial >= total){
        Serial1.print("page Menu");
        Serial1.write(termination, 3);
        return pronto;
    }

    return no_event;
}

void reiniciar_controle(){
    erro_integral = 0;
    erro_anterior = 0;
}

void lei_de_controle(){
    forca_z = loadcell_z.get_value(5);
    float erro = referencia_forca_z - forca_z;
    erro_integral += 0.5*(erro + erro_anterior)*prd_controle;

    go_to_z_mm(kp*erro + ki*erro_integral);

    erro_anterior = erro;
}