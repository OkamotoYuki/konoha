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
	DTask task1 = new DTask(1, "test", 1, "eval", logpoolip, makeFibo(36));
	DTask task2 = new DTask(2, "test", 2, "eval", logpoolip, makeFibo(37));
	DTask task3 = new DTask(3, "test", 3, "eval", logpoolip, makeFibo(38));
	DTask task4 = new DTask(4, "test", 4, "eval", logpoolip, makeFibo(39));
	DTask task5 = new DTask(5, "test", 5, "eval", logpoolip, makeFibo(40));
	DTask task6 = new DTask(6, "test", 6, "eval", logpoolip, makeFibo(41));
	dse.add(task1, false);
	dse.add(task2, false);
	dse.add(task3, false);
	dse.add(task4, false);
	dse.add(task5, false);
	dse.add(task6, false);
	dse.apply();
}

main();
