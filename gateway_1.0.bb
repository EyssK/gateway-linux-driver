SUMMARY = "Gateway driver as a script"
LICENSE = "CLOSED"

SRC_URI = "file://gateway.sh \
           file://Makefile \
           file://gateway.c \
          "

RDEPENDS_${PN} += "bash"

do_compile() {
	${CC} -o ${WORKDIR}/gateway_mmap ${WORKDIR}/gateway.c
}

do_install() {
        install -m 0755 -d ${D}/home/root/
        install -m 0755 ${WORKDIR}/gateway.sh ${D}/home/root/
        install -m 0755 ${WORKDIR}/gateway_mmap ${D}/home/root/
}

FILES_${PN} += "/home/root/"
