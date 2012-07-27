K.import("dse");

void main() {
	String webHost = "web";
	String dbHost = "db";
	int port = 8080;
	String logpoolip = "0.0.0.0";
	LogPool lp = new LogPool(logpoolip, 14801);
	lp.loadFile("dump", "query.k");

	DSE web = new DSE(webHost, port);
	DSE db = new DSE(dbHost, port);
	DTask resourceMonitor = new DTask(1, "test", 1, "eval", logpoolip, "");
	resourceMonitor.readFile("./resourceMonitor.k")
	DTask httpdRestart = new DTask(2, "test", 2, "eval", logpoolip, "");
	httpdRestart.readFile("./httpdRestart.k");
	DTask mysqldRestart = new DTask(3, "test", 3, "eval", logpoolip, "");
	mysqldRestart.readFile("./mysqldRestart.k");
	web.add(resourceMonitor, true);
	web.apply();

	while(true) {
		Log log = lp.get();
		if(log.isNull()) break;
		p(log);
		if(log.get("cpu_sys") == "10" || log.get("cpu_sys") == "11" || log.get("cpu_sys") == "12") break;
//		if((int)log.get("cpu_sys") > 10) break;
	}

	db.add(mysqldRestart, true);
	web.add(httpdRestart, false);
	DSE[] dses = new DSE[0];
	dses.add(db);
	dses.add(web);
	DSEController.sync(dses);
}

main();
