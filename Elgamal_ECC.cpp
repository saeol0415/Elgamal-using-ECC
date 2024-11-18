#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define CURVE_A 2
#define CURVE_B 3
#define MODULO_P 521

#define Gx 3
#define Gy 6
#define DECRYPTOR 15

#define MAX_MESSAGE_SIZE 100

typedef struct {
    int x;
    int y;
} Point;

int mod(int a, int p) {
    int result = a % p;
    if (result < 0) result += p;
    return result;
}

int mod_inverse(int a, int p) {
    int t = 0, newt = 1;
    int r = p, newr = a;
    while (newr != 0) {
        int quotient = r / newr;
        int temp = t;
        t = newt;
        newt = temp - quotient * newt;
        temp = r;
        r = newr;
        newr = temp - quotient * newr;
    }
    if (r > 1) return -1;   // 역원이 없음
    if (t < 0) t += p;
    return t;
}

Point negate_point(Point P) {
    return (Point){P.x, mod(MODULO_P - P.y, MODULO_P)};
}

Point point_double(Point P) {
    if (mod(P.y, MODULO_P) == 0) return (Point){0, 0};
    int m = mod(3 * P.x * P.x + CURVE_A, MODULO_P) * mod_inverse(mod(2 * P.y, MODULO_P), MODULO_P);
    m = mod(m, MODULO_P);
    Point R;
    R.x = mod(m * m - 2 * P.x, MODULO_P);
    R.y = mod(m * (P.x - R.x) - P.y, MODULO_P);
    return R;
}

Point point_add(Point P, Point Q) {
    if (mod(P.x, MODULO_P) == mod(Q.x, MODULO_P)) {
        if (mod(P.y, MODULO_P) == mod(Q.y, MODULO_P)) return point_double(P);
        else return (Point){0, 0};
    }
    if (P.x == 0 && P.y == 0) return Q;
    else if (Q.x == 0 && Q.y == 0) return P;
    int m = mod(Q.y - P.y, MODULO_P) * mod_inverse(mod(Q.x - P.x, MODULO_P), MODULO_P);
    m = mod(m, MODULO_P);
    Point R;
    R.x = mod(m * m - P.x - Q.x, MODULO_P);
    R.y = mod(m * (P.x - R.x) - P.y, MODULO_P);
    return R;
}

Point scalar_multiply(Point A, int k) {
    if (k < 0) k = -k;
    Point R = {0, 0};   // 무한 원점을 (0, 0)로 나타냄
    Point Addend = A;
    while (k > 0) {
        if (k%2==1) {
            if (R.x == 0 && R.y == 0) {
                R = Addend;
            } else {
                R = point_add(R, Addend);
            }
        }
        Addend = point_double(Addend);
        k /= 2;
    }
    if (k < 0) R = negate_point(R);
    return R;
}

int is_valid_point(int x, int y) {
    int lhs = mod(y * y, MODULO_P);
    int rhs = mod(x * x * x + CURVE_A * x + CURVE_B, MODULO_P);
    return lhs == rhs;
}

void generate_curve_points(Point* Points, int* point_count) {
    *point_count = 0;
    for (int x = 0; x < MODULO_P; x++) {
        for (int y = 0; y < MODULO_P; y++) {
            if (is_valid_point(x, y)) {
                Points[*point_count].x = x;
                Points[*point_count].y = y;
                (*point_count)++;
                if (*point_count >= 52) {
                    return; // 대소문자 알파벳 대응에 필요한 점만 찾으면 종료
                }
            }
        }
    }
}

Point ascii_to_point(char c, Point* Points) {
    // 대소문자 알파벳만 대응
    if (c >= 'A' && c <= 'Z') {
        return Points[c - 'A']; // 대문자는 0~25번 인덱스
    } else if (c >= 'a' && c <= 'z') {
        return Points[26 + (c - 'a')]; // 소문자는 26~51번 인덱스
    } else return (Point){-1, -1}; // 유효하지 않은 점 반환
}

char point_to_ascii(Point P, Point* Points, int point_count) {
    for (int i = 0; i < point_count; i++) {
        if (Points[i].x == P.x && Points[i].y == P.y) {
            if (i < 26) {
                return 'A' + i; // 대문자
            } else if (i < 52) {
                return 'a' + (i - 26); // 소문자
            }
        }
    }
    return '?'; // 유효하지 않은 점에 대해 '?' 반환
}

void encrypt(Point M, int k, Point *C1, Point *C2) {
    Point G = {Gx, Gy};
    Point dG = scalar_multiply(G, DECRYPTOR);
    Point kdG = scalar_multiply(dG, k);
    *C1 = scalar_multiply(G, k);
    *C2 = point_add(M, kdG);
}

Point decrypt(Point C1, Point C2) {
    Point dC1 = scalar_multiply(C1, DECRYPTOR);
    Point neg_dC1 = negate_point(dC1);
    Point M = point_add(C2, neg_dC1);
    return M;
}

int main() {
    char message[MAX_MESSAGE_SIZE];
    Point C1, C2, M, Points[52];
    int point_count = 0;
    char decrypted_string[sizeof(message)];

    scanf("%s", message);

    generate_curve_points(Points, &point_count);
    
    if (point_count < 52) {
        printf("Lacking valid points. Try bigger p value or choose different elliptic curve.\n");
        return -1;
    }

    srand(time(NULL));
    int k = mod(rand(), MODULO_P - 1) + 1;
    printf("Current k = %d\n", k);

    for (int i = 0; message[i] != '\0'; i++) {
        Point p = ascii_to_point(message[i], Points);
        if (p.x != -1 && p.y != -1) {
            printf("'%c' => (%d, %d)\n", message[i], p.x, p.y);
        } else {
            printf("Character '%c' is not valid.\n", message[i]);
        }
    }

    for (int i = 0; message[i] != '\0'; i++) {
        M = ascii_to_point(message[i], Points);
        encrypt(M, k, &C1, &C2);
        Point decrypted = decrypt(C1, C2);
        decrypted_string[i] = point_to_ascii(decrypted, Points, point_count);

        if (i == 0) printf("C1: (%d, %d)\nC2: ", C1.x, C1.y);
        printf("(%d, %d) ", C2.x, C2.y);
    }
    decrypted_string[strlen(message)] = '\0';
    printf("\nPlain text: %s", decrypted_string);
    return 0;
}