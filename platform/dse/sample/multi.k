K.import("dse");

String makeFibo(int i) {
	String fibo = "K.import(\"konoha\"); int fibo(int i) { if(i < 3) return i; else return fibo(i - 1) + fibo(i - 2); }; p(fibo(" + i + "));";
	return fibo;
}

void main() {
	String host = "localhost";
	int port = 8080;
	String logpoolip = "0.0.0.0";
	DSE dse = new DSE(host, port);
	DTask t1 = new DTask(1, "test", 1, "eval", logpoolip, makeFibo(36));
	DTask t2 = new DTask(2, "test", 2, "eval", logpoolip, makeFibo(37));
	DTask t3 = new DTask(3, "test", 3, "eval", logpoolip, makeFibo(38));
	DTask t4 = new DTask(4, "test", 4, "eval", logpoolip, makeFibo(39));
	DTask t5 = new DTask(5, "test", 5, "eval", logpoolip, makeFibo(40));
	DTask t6 = new DTask(6, "test", 6, "eval", logpoolip, makeFibo(41));
	dse.add(t1, false);
	dse.add(t2, false);
	dse.add(t3, false);
	dse.add(t4, false);
	dse.add(t5, false);
	dse.add(t6, false);
	dse.apply();
}

main();
