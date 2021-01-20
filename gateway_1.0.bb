SUMMARY = "Gateway driver as a script"
LICENSE = "CLOSED"

SRC_URI = "file://gateway.sh"

RDEPENDS_${PN} += "bash"

do_install() {
        install -m 0755 -d ${D}/home/root/
        install -m 0755 ${WORKDIR}/gateway.sh ${D}/home/root/
}

FILES_${PN} += "/home/root/"
