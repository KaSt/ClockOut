Name:           clockout
Version:        1.0.0
Release:        1%{?dist}
Summary:        Terminal companion for tracking the rest of your workday

License:        Custom
URL:            https://example.com/clockout
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make, ncurses-devel, json-c-devel
Requires:       ncurses, json-c

%description
ClockOut is a terminal-based companion for tracking the rest of your workday.
It renders a large countdown timer or discrete blocks that show how long
remains in your scheduled shift, lunch, and breaks.

%prep
%setup -q

%build
%{__make}

%install
rm -rf %{buildroot}
install -Dm755 clockout %{buildroot}/usr/local/bin/clockout
install -Dm644 README.md %{buildroot}/usr/share/doc/%{name}/README.md

%files
/usr/local/bin/clockout
/usr/share/doc/%{name}/README.md

%changelog
* Thu Jan 01 1970 ClockOut Maintainers <maintainers@example.com> - 1.0.0-1
- Initial package
