int g = 10;

int fun()
{
	int a = 1, b = a + 2, c = a * b, d = b * c, e = 10;

	if(a > b) {
		c = a * b;
		d = b / c;
		e = c % d;
	}

    for (int i = 10; i >= 0; --i) {
        c = a * b;
		d = b * c;
		e = c * d;
    }

    return a + e;
}

int main() {
	int i = 500, j;

	while(i--){
		j = fun();
	}

	return (i = j) + g;
}
