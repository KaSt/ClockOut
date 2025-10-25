EAPI=8

DESCRIPTION="Terminal companion for tracking the rest of your workday"
HOMEPAGE="https://example.com/clockout"
SRC_URI="clockout-${PV}.tar.gz"

LICENSE="all-rights-reserved"
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

DEPEND="sys-libs/ncurses dev-libs/json-c"
RDEPEND="${DEPEND}"

S="${WORKDIR}/clockout-${PV}"

src_compile() {
emake
}

src_install() {
dobin clockout
dodoc README.md
}
