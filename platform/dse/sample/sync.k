K.import("dse");

String makeFibo(int i) {
	String fibo = "K.import(\"konoha\"); int fibo(int i) { if(i < 3) return i; else return fibo(i - 1) + fibo(i - 2); }; p(fibo(" + i + "));";
	return fibo;
}

void main() {
	String host = "localhost";
	int port = 8080;
	String logpoolip = "0.0.0.0";
	DSE dse0 = new DSE(host, port);
	DSE dse1 = new DSE(host, port);
	DTask task0 = new DTask(0, "test", 0, "eval", logpoolip, makeFibo(40));
	DTask task1 = new DTask(1, "test", 1, "eval", logpoolip, makeFibo(1));
	dse0.add(task0, true);
	dse1.add(task1, false);
	DSE[] dses = new DSE[0];
	dses.add(dse0);
	dses.add(dse1);
	DSEController.apply(dses);
//	DSEController.sync(dses);
}

main();
