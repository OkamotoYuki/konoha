K.import("konoha");
K.import("dse");

String makeInfiniteLoop() {
	String loop = "K.import(\"konoha.while\"); K.import(\"posix.process\"); void main() { while(true) { System.sleep(1); } }\nmain();";
	return loop;
}

int main() {
	String host = "localhost";
	int port = 8080;
	String logpoolip = "0.0.0.0";
	DSE dse = new DSE(host, port);
	DTask loop = new DTask(0, "loop", 0, "eval", logpoolip, makeInfiniteLoop());
	dse.add(loop, false);
	dse.apply();
}

main();
