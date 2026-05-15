int x;
int y;
int result;

int add(int a)
{
    int b;
    b = a + 10;
    return b;
}

int main()
{
    x = 5;
    y = 3;
    result = x + y;

    if(result > 6)
    {
        x = x + 1;
    }
    else
    {
        y = y + 1;
    }

    while(x < 10)
    {
        x = x + 1;
    }

    printf("Result: %d", result);

    return 0;
}