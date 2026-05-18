/* multi-line comment
   এটা ignore হবে */

// single line comment

struct Point {
    int x;
    int y;
};

int arr[5];
int *ptr;

int add(int a, int b) {
    return a + b;
}

int square(int n) {
    return n * n;
}

int main() {
    /* variables */
    int x = 10;
    int y = 20;
    int z;

    /* array */
    arr[0] = 5;
    arr[1] = x + 3;

    /* pointer */
    ptr = &x;
    z = *ptr;

    /* function call */
    z = add(x, y);

    /* && and || TAC */
    int a = 1;
    int b = 0;
    int c;
    c = a && b;
    c = a || b;

    /* if-else */
    if (x > y) {
        z = x - y;
    } else {
        z = y - x;
    }

    /* while */
    while (x > 0) {
        x = x - 1;
    }

    /* for loop */
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 2;
    }

    /* switch-case */
    switch (z) {
        case 1:
            x = 100;
            break;
        case 2:
            x = 200;
            break;
        default:
            x = 0;
            break;
    }

    printf("Result: %d", z);

    return z;
}