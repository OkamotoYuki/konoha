K.import("konoha");
K.import("dse");

String makeKillTask(int pid) {
	String kill = "K.import(\"konoha.bytes\"); K.import(\"konoha.subproc\"); void main() { Subproc sp = new Subproc(\"kill " + pid + "\", false); sp.fg(); }\nmain();";
	return kill;
}

int main() {
	String host = "localhost";
	int port = 8080;
	String logpoolip = "0.0.0.0";
	int pid = 8024;
	DSE dse = new DSE(host, port);
	DTask kill = new DTask(1, "loop", 1, "eval", logpoolip, makeKillTask(pid));
	dse.add(kill, false);
	dse.apply();
}

main();
