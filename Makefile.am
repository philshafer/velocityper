ACLOCAL_AMFLAGS = -I m4

SUBDIRS= src

dist_doc_DATA = Copyright

EXTRA_DIST = \
    warnings.mk \
    README.rst \
    src/velocityper.1 \
    src/velocityper.c \
    velocityper.pc

.PHONY: test tests

GH_PAGES_DIR = gh-pages/
GH_PAGES_DIR_VER = gh-pages/${PACKAGE_VERSION}
PACKAGE_FILE = ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = ${PACKAGE_NAME}.pc

get-wiki:
	git clone https://github.com/philshafer/${PACKAGE_NAME}.wiki.git wiki

get-gh-pages:
	git clone https://github.com/philshafer/${PACKAGE_NAME}.git \
		gh-pages -b gh-pages

UPDATE_PACKAGE_FILE = \
    -e "s;__SHA1__;$$SHA1;" \
    -e "s;__SHA256__;SHA256 (textproc/${PACKAGE_FILE}) = $$SHA256;" \
    -e "s;__SIZE__;SIZE (textproc/${PACKAGE_FILE}) = $$SIZE;"

GH_PACKAGING_DIR = ${PACKAGE_VERSION}/packaging
GH_PAGES_PACKAGE_DIR = ${GH_PAGES_DIR}/${GH_PACKAGING_DIR}

packages:
	@-[ -d ${GH_PAGES_DIR} ] && set -x \
		&& echo "Updating packages on gh-pages ..." \
		&& mkdir -p ${GH_PAGES_DIR}/${GH_PACKAGING_DIR} \
		&& cp packaging/${PACKAGE_NAME}.pc \
			${GH_PAGES_PACKAGE_DIR}/${PACKAGE_NAME}.pc \
		&& (cd ${GH_PAGES_DIR} \
			&& git add ${GH_PACKAGING_DIR} \
			&& git commit -m 'new packaging data' \
			       ${GH_PACKAGING_DIR} \
			&& git push origin gh-pages ) ; true
