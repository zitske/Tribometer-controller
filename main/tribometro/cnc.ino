const int PULSE_X = 5;
const int PULSE_Y = 8;
const int PULSE_Z = 13;
const int DIR_X =  4;
const int DIR_Y =  7;
const int DIR_Z = 11;

typedef struct{
    uint8_t xyz = 0;
    unsigned short n = 1;
} Incremento;

typedef struct node{ // Lista linkada com os pulsos, -1 equivale a um pulso negativo, 0 a nenhum e 1 a um pulso positivo. n é a contagem de pulsos
    Incremento data;
    struct node *proximo = NULL;
} Node;

int x_atual = 0, y_atual = 0, z_atual = 0; // Posição em x, y e z em pulsos
int x_real = 0, y_real = 0, z_real = 0;
const int passos_por_mm = 40;

// Começo e fim da fila circular
Node *primeiro = NULL;
Node *ultimo = NULL;

int cnc_run = 1;

int cnc_complete(){ // Retorna 1 se não houver nenhum incremento na fila de execução
    if(primeiro == NULL && ultimo == NULL){
        return 1;
    }else{
        return 0;
    }
}

void limpar_fila(){
    cli();
    while(primeiro != NULL){
        remover_incremento();
    }
    sei();
}

void adicionar_incremento(short x, short y, short z, unsigned int n){ // Adiciona um incremento à fila de execução
    Node *newnode = (Node *)malloc(sizeof(Node));

    if(newnode == NULL){
        Serial.println("falha ao adicionar incremento");
        return;
    }

    Incremento temp;
    temp.xyz = (x>0?1:0 << 0) | (x!=0?1:0 << 1) | (y>0?1:0 << 2) | (y!=0?1:0 << 3) | (z>0?1:0 << 4) | (z!=0?1:0 << 5);
    temp.n = n;

    x_atual += x*n;
    y_atual += y*n;
    z_atual += z*n;

    newnode->data = temp;
    newnode->proximo = NULL;

    if(primeiro == NULL && ultimo == NULL){
        primeiro = newnode;
        ultimo = newnode;
        return;
    }

    ultimo->proximo = newnode;
    ultimo = newnode;

    ultimo->proximo = primeiro; // Conecta o final da lista ao primeiro elemento
}

void remover_incremento(){ // Remove o primeiro incremento da fila de execução
    if(primeiro == NULL && ultimo == NULL){ // Se não houver nenhum elemento na lista
        return;
    }

    if(primeiro == ultimo){ // Se houver somente um elemento na lista
        free(primeiro);
        primeiro = NULL;
        ultimo = NULL;
        return;
    }

    Node *temp = primeiro->proximo;
    free(primeiro);
    primeiro = temp;

    ultimo->proximo = primeiro; // Conecta o final da lista ao primeiro elemento
}

int i = 0;

void atualizar_cnc(){ // No primeiro semiciclo, gera os pulsos equivalentes ao primeiro elemento da fila e o remove se completo, e no segundo semiciclo zera os valores das saídas digitais
    if(i == 1){ // Quando i == 0, ler a fila e gerar as bordas de subida, quando i == 1, zerar os valores de saída para permitir a próxima borda. i alterna entre 0 e 1
        digitalWrite(PULSE_X, LOW);
        // digitalWrite(DIR_X, LOW);
        digitalWrite(PULSE_Y, LOW);
        // digitalWrite(DIR_Y, LOW);
        digitalWrite(PULSE_Z, LOW);
        // digitalWrite(DIR_Z, LOW);
        i = 0;
        return;
    }else{
        i = 1;
    }

    if(primeiro == NULL){
        return;
    }

    if((primeiro->data).n > 0){ // 
        digitalWrite(DIR_X, bitRead((primeiro->data).xyz, 0));
        digitalWrite(DIR_Y, bitRead((primeiro->data).xyz, 2));
        digitalWrite(DIR_Z, bitRead((primeiro->data).xyz, 4));

        delayMicroseconds(20);

        digitalWrite(PULSE_X, bitRead((primeiro->data).xyz, 1));
        digitalWrite(PULSE_Y, bitRead((primeiro->data).xyz, 3));
        digitalWrite(PULSE_Z, bitRead((primeiro->data).xyz, 5));

        x_real += (bitRead((primeiro->data).xyz, 0)>0?1:-1)*bitRead((primeiro->data).xyz, 1);
        y_real += (bitRead((primeiro->data).xyz, 2)>0?1:-1)*bitRead((primeiro->data).xyz, 3);
        z_real += (bitRead((primeiro->data).xyz, 4)>0?1:-1)*bitRead((primeiro->data).xyz, 5);

        // x_atual += (primeiro->data).x;
        // y_atual += (primeiro->data).y;
        // z_atual += (primeiro->data).z;

        // Serial.print("pulso: ");
        // Serial.print((primeiro->data).x);
        // Serial.print(" ");
        // Serial.print((primeiro->data).y);
        // Serial.print(" ");
        // Serial.print((primeiro->data).z);
        // Serial.print(" ");
        // Serial.println((primeiro->data).n);

        (primeiro->data).n = (primeiro->data).n - 1;

        if((primeiro->data).n == 0){ // Se o incremento estiver completo, remove ele da fila
            remover_incremento();
        }
    }else{
        remover_incremento();
    }
}

inline int sgn(int x){ // Retorna o sinal do número, ou zero
    if(x > 0){
        return 1;
    }else if(x < 0){
        return -1;
    }
    return 0;
}

inline float distancia_ponto_e_reta(float a, float b, float c, float x, float y){
    return abs(a*x + b*y + c);
}

void go_to_mm(float x, float y){
    go_to(x*passos_por_mm, y*passos_por_mm);
}

void go_to(int x, int y){
    float a = y_atual - y, b = x - x_atual, c = (x_atual*y) - (x*y_atual);
    float h = 1/hypot(a, b);
    a *= h;
    b *= h;
    c *= h;

    int delta_x = x_atual - x, delta_y = y_atual - y;
    int reto_x = 0, reto_y = 0, reto_total = 0;
    int diag_x = 0, diag_y = 0, diag_total = 0;

    diag_total = min(abs(delta_x), abs(delta_y));
    diag_x = -sgn(delta_x);
    diag_y = -sgn(delta_y);

    reto_total = max(abs(delta_x + diag_x*diag_total), abs(delta_y + diag_y*diag_total));
    reto_x = -sgn(delta_x + diag_x*diag_total);
    reto_y = -sgn(delta_y + diag_y*diag_total);

    int x_sim = x_atual, y_sim = y_atual;

    int max_step = 5;

    while(abs(y_sim - y) > 0 || abs(x_sim - x)){
        if(reto_total > 0 && diag_total > 0){
            if(distancia_ponto_e_reta(a, b, c, x_sim + diag_x, y_sim + diag_y) > distancia_ponto_e_reta(a, b, c, x_sim + reto_x, y_sim + reto_y)){
                int delta = min(reto_total, max_step);
                adicionar_incremento(reto_x, reto_y, 0, delta);
                reto_total -= delta;
                x_sim += reto_x*delta;
                y_sim += reto_y*delta;
            }else{
                int delta = min(diag_total, max_step);
                adicionar_incremento(diag_x, diag_y, 0, delta);
                diag_total -= delta;
                x_sim += diag_x*delta;
                y_sim += diag_y*delta;
            }
        }else{
            int delta = min(reto_total, max_step);
            if(delta != 0){
                adicionar_incremento(reto_x, reto_y, 0, delta);
                reto_total -= delta;
                x_sim += reto_x*delta;
                y_sim += reto_y*delta;
            }

            delta = min(diag_total, max_step);
            if(delta != 0){
                adicionar_incremento(diag_x, diag_y, 0, delta);
                diag_total -= delta;
                x_sim += diag_x*delta;
                y_sim += diag_y*delta;
            }
        }
    }
}

const float pi = 3.1415926358;

void go_to_circular_mm(float arco_radianos, float xc, float yc, int steps){
    go_to_circular(arco_radianos, xc*passos_por_mm, yc*passos_por_mm, steps);
}

void go_to_circular(float arco_radianos, int xc, int yc, int steps){
    float raio = hypot(x_atual - xc, y_atual - yc);
    float angulo_atual = atan2(y_atual - yc, x_atual - xc);
    float angulo_step = arco_radianos/steps;

    int i;
    for(i = 0; i < steps; i++){
        angulo_atual += angulo_step;
        go_to(xc + round(raio*cos(angulo_atual)), yc + round(raio*sin(angulo_atual)));
    }
}

void go_to_z_mm(float z){
    go_to_z(z*passos_por_mm);
}

void go_to_z(int z){
    int delta_z = z - z_atual;
    adicionar_incremento(0, 0, sgn(delta_z), abs(delta_z));
}

long periodo_atualizacao_cnc(int velocidade){ // tempo de atualização do cnc em microssegundos a partir da velocidade em mm/s
    //passos_por_seg = velocidade*passos_por_mm;
    //periodo_em_us = 1e6/passos_por_seg;
    //periodo_real = periodo_em_us/2 (considerando que um periodo gera a borda positiva e o outro zera)
    return 500000/(velocidade*passos_por_mm);
}