/*
* modo manual funciona
 */
#include "HX711.h" /*Biblioteca do HX711.h */

#define HMI_RX 2
#define HMI_TX 3


#define LCELL_Y_DT 44
#define LCELL_Y_CK 42
#define LCELL_Z_DT 40
#define LCELL_Z_CK 38

HX711 balanca;                                                                      

float calibration_factorZ = 33340.00;                                                       
float pesoZ;                                                                               
float data = 0.16;

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

estado_t estado_anterior = standby, estado_atual = standby, proximo_estado;
dados_fadiga_t dados_fadiga;
dados_linear_t dados_linear;
dados_riscamento_t dados_riscamento;
dados_rotativo_t dados_rotativo;

long last_maquina_de_estados = 0;
long last_cnc = 0;

long tempo_cnc = 100; // Tempo de atualização do cnc em microssegundos, a alteração deste valor afeta a velocidade.

char cmd_buffer[10];

void setup() {
  // Inicialização das portas seriais

  Serial.begin(115200);
  Serial1.begin(9600);
  balanca.begin(LCELL_Z_DT, LCELL_Z_CK);
  balanca.set_scale(-289590);                                                                      /* seta escala */
  balanca.tare();      // velocidade inicial de 100 mm/s
  long zero_factor = balanca.read_average();
  pesoZ = balanca.get_units(), 10; 
}

void loop() {
  pesoZ = balanca.get_units(), 10;
  //Serial.println(pesoZ); 
    if(millis() - last_maquina_de_estados > 100){
        maquina_de_estados();
        last_maquina_de_estados = millis();
    }

}

extern int x_atual, y_atual, z_atual; // Posição em x, y e z em pulsos

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
                    Serial.println("G10 P0 L20 X0 Y0 Z0");
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
    
     //Serial.print((int)estado_atual);
     //Serial.print(" ");
     //Serial.print((int)evento);
     //Serial.print(" ");
     //Serial.println((int)proximo_estado);

    estado_anterior = estado_atual;
    estado_atual = proximo_estado;
}

evento_t standby_f(){
    if(estado_anterior != standby){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        //Serial.println("Standby");
    }

    if(Serial1.available() > 0){ // Se houverem dados a serem lidos do HMI, ler os três primeiros, que serão equivalentes ao cabeçalho do comando e se forem válidos, ler os dados para cada comando
        size_t numero_de_lidos = Serial1.readBytes((char *)cmd_buffer, 3);
        if(numero_de_lidos == 3){
            cmd_buffer[3] = '\0';
            if(strcmp(cmd_buffer, "lin") == 0){ // Comando para iniciar o modo linear
                numero_de_lidos = Serial1.readBytes((char *)&dados_linear, 4);
                if(numero_de_lidos == 4){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_linear;
                }
            }else if(strcmp(cmd_buffer, "rot") == 0){ // Comando para iniciar o modo rotação
                numero_de_lidos = Serial1.readBytes((char *)&dados_rotativo, 4);
                if(numero_de_lidos == 4){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_rotativo;
                }
            }else if(strcmp(cmd_buffer, "ris") == 0){ // Comando para iniciar o modo riscamento
                numero_de_lidos = Serial1.readBytes((char *)&dados_riscamento, 5);
                if(numero_de_lidos == 5){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_riscamento;
                }
            }else if(strcmp(cmd_buffer, "fad") == 0){ // Comando para iniciar o modo fadiga
                numero_de_lidos = Serial1.readBytes((char *)&dados_fadiga, 6);
                if(numero_de_lidos == 6){ // Checa se os dados foram lidos corretamente, somente ir à página Load se sim
                    Serial1.print("page Load");
                    Serial1.write(termination, 3);
                    return iniciar_fadiga;
                }
            }else if(strcmp(cmd_buffer, "man") == 0){ // Comando para iniciar o modo manual
                return iniciar_manual;
            }else if(strcmp(cmd_buffer, "zro") == 0){ // Comando zerar
                return zerar;
            }
        }
    }

    return no_event; // caso não haja dados na porta serial ou ocorra algum erro, não há evento emitido e o sistema continua em standby
}

long tempo_inicial = 0, tempo_step;
const long step = 1000, total = 5000;

evento_t linear_alternativo_f(){
    if(estado_anterior != linear_alternativo){ // Esta condição indica que o sistema acabou de entrar no estado, portanto é onde ocorre a inicialização
        /*Serial.println("Modo Linear");
        Serial.print("Forca Normal (N): ");
        Serial.println(dados_linear.normal);
        Serial.print("Curso (mm): ");
        Serial.println(dados_linear.curso);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_linear.distancia);
        */
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
        Serial.println("Modo Rotativo");
        Serial.print("Forca Normal (N): ");
        Serial.println(dados_rotativo.normal);
        Serial.print("Diametro (mm): ");
        Serial.println(dados_rotativo.diametro);
        Serial.print("Distancia Total (m): ");
        Serial.println(dados_rotativo.distancia);
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

extern const int passos_por_mm;

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
                Serial.println("G21G90 G0Z5");
                Serial.println("G90 G0 X0 Y0");
                Serial.println("G90 G0 Z0");
            }else if(strcmp(cmd_buffer, "x++") == 0){
                Serial.println("G21G91G1X2F100");
                Serial.println("G90 G21");

            }else if(strcmp(cmd_buffer, "x--") == 0){
                Serial.println("G21G91G1X-2F100");
                Serial.println("G90 G21");

            }else if(strcmp(cmd_buffer, "y++") == 0){
                Serial.println("G21G91G1Y2F100");
                Serial.println("G90 G21");

            }else if(strcmp(cmd_buffer, "y--") == 0){
                Serial.println("G21G91G1Y-2F100");
                Serial.println("G90 G21");

            }else if(strcmp(cmd_buffer, "z++") == 0){
                Serial.println("G21G91G1Z2F100");
                Serial.println("G90 G21");

            }else if(strcmp(cmd_buffer, "z--") == 0){
                Serial.println("G21G91G1Z-2F100");
                Serial.println("G90 G21");

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