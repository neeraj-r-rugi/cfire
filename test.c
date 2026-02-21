#include <stdio.h>
int main(int argc, char * argv[]) {
    char text[128];
    printf("Enter some text: ");
    scanf("%127s", text);
    printf("Hello, World!: %s, %s %s %s %s\n", text, argv[1], argv[2], argv[3], argv[4]);
    return 0;
}
