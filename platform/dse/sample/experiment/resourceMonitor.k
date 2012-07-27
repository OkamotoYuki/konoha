K.import("konoha.assignment");
K.import("konoha.subproc");

void monitor() {
	Subproc s = Subproc.new("/usr/bin/dvmstat --logpool=0.0.0.0 --context=1 1", false);
	s.bg();
}

monitor();
