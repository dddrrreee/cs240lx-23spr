static int foo(bar) {
        if (bar) goto err;
	if (baz < 0) {
err:
            bad;
	}
	return bar;
}
