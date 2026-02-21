#include <stdio.h>
int main() {
    char text[128];
    printf("Enter some text: ");
    scanf("%127s", text);
    printf("Hello, World!: %s\n", text);
    return 0;
}