# !/bin/bash
#
# prep_release_text.sh -- generate copy paste text sippets for a client github release
#
# (C) 2020 jw@owncloud.com


tag=$1

if [ -z "$tag" -o "$tag" == '-h' -o "$tag" == '--help' ]; then
  cat <<EOF
Usage: $0 CLIENT_REPO_TAG [OWNCLOUD_DOWNLOAD_URL [TESTPILOT_DOWNLOAD_URL]]


This creates a folder with the same name as the CLIENT_REPO_TAG containing
- github_release.txt	text suggestion for a github release with download links and changelog

If the URLs are ommited, the latest additions to the 
download folders (testing if the tag looks like a beta version else stable) are used that match the tag.
EOF
  exit 1
fi

clean_href() {
  # convert <a href="./2.7.0.2146-v270beta5/">
  #    into 2.7.0.2146-v270beta5
  sed -e 's@.*href[="\./]*@@' -e 's@[/"> \t].*$@@'
}

ucbeta() {
  echo "$1" | sed -e 's/beta/Beta/' -e 's/rc/RC/'
}

  
# if the tag has a -, then the part behind is the beta or rc match
vers=$(echo $tag | sed -e 's/^v//' -e 's/-.*//')
beta=$(echo $tag | sed -e 's/^.*-//')
if [ "$beta" == "$tag" ]; then beta= ; fi

if [ -n "$2" ]; then
  oc_dl="$2"
  tp_dl="$3"	# or empty
else
  if [ -n "$beta" ]; then
    echo "tag looks like a beta or rc release, using testing URLs."
    oc_dl=https://download.owncloud.com/desktop/ownCloud/testing/
    tp_dl=https://download.owncloud.com/desktop/testpilotcloud/testing/
  else
    echo "tag looks like a final, using stable URLs."
    oc_dl=https://download.owncloud.com/desktop/ownCloud/stable/
    tp_dl=https://download.owncloud.com/desktop/testpilotcloud/stable/
  fi
  echo ... $oc_dl
  echo ... $tp_dl

  re_vers=$(echo $vers | sed -e 's/\./\\./g')
  re_beta=$(echo $beta | sed -e 's/\./\\./g')
  
  # <a href="./2.7.0.2146-v270beta5/">   -> 2.7.0.2146-v270beta5
  oc_subdir=$(curl -k -L -s $oc_dl | grep href | grep "$re_vers.*$re_beta" | tail -1 | clean_href)
  tp_subdir=$(curl -k -L -s $tp_dl | grep href | grep "$re_vers.*$re_beta" | tail -1 | clean_href)
  oc_dl=$oc_dl$oc_subdir
  tp_dl=$tp_dl$tp_subdir
  # echo oc_dl: $oc_dl
  # echo tp_dl: $tp_dl
fi

oc_win32_msi=$(    curl -k -s -L $oc_dl/win | grep "href.*x86\.msi"      | clean_href)
oc_win32_GPO_msi=$(curl -k -s -L $oc_dl/win | grep "href.*x86\.GPO\.msi" | clean_href)
oc_win64_msi=$(    curl -k -s -L $oc_dl/win | grep "href.*x64\.msi"      | clean_href)
oc_win64_GPO_msi=$(curl -k -s -L $oc_dl/win | grep "href.*x64\.GPO\.msi" | clean_href)

oc_mac_pkg=$(      curl -k -s -L $oc_dl/mac | grep href.*pkg | clean_href)
oc_linux_dl=$(     curl -k -s -L $oc_dl/linux | grep href.*download | clean_href)

oc_source=$(       curl -k -s -L $oc_dl/source | grep href.*tar.xz | clean_href)
oc_source_asc=$(   curl -k -s -L $oc_dl/source | grep href.*tar.xz.asc | clean_href)

tp_win32_msi=$(    curl -k -s -L $tp_dl/win | grep "href.*x86\.msi"      | clean_href)
tp_win32_GPO_msi=$(curl -k -s -L $tp_dl/win | grep "href.*x86\.GPO\.msi" | clean_href)
tp_win64_msi=$(    curl -k -s -L $tp_dl/win | grep "href.*x64\.msi"      | clean_href)
tp_win64_GPO_msi=$(curl -k -s -L $tp_dl/win | grep "href.*x64\.GPO\.msi" | clean_href)

tp_mac_pkg=$(      curl -k -s -L $tp_dl/mac | grep href.*pkg | clean_href)
tp_linux_dl=$(     curl -k -s -L $tp_dl/linux | grep href.*download | clean_href)

mkdir -p $tag
cat <<EOF > $tag/env.sh
tag=$tag
vers=$vers
beta=$beta
oc_dl=$oc_dl
tp_dl=$tp_dl

oc_win32_msi=$oc_win32_msi
oc_win32_GPO_msi=$oc_win32_GPO_msi
oc_win64_msi=$oc_win64_msi
oc_win64_GPO_msi=$oc_win64_GPO_msi
oc_mac_pkg=$oc_mac_pkg
oc_linux_dl=$oc_linux_dl

oc_source=$oc_source
oc_source_asc=$oc_source_asc

tp_win32_msi=$tp_win32_msi
tp_win32_GPO_msi=$tp_win32_GPO_msi
tp_win64_msi=$tp_win64_msi
tp_win64_GPO_msi=$tp_win64_GPO_msi
tp_mac_pkg=$tp_mac_pkg
tp_linux_dl=$tp_linux_dl
EOF

oc_txt=""
test -n "$oc_win64_msi"     && oc_txt="$oc_txt [Windows MSI]($oc_dl/win/$oc_win64_msi) |"
test -n "$oc_win32_msi"     && oc_txt="$oc_txt [32 bit]($oc_dl/win/$oc_win32_msi) |"
test -n "$oc_win64_GPO_msi" && oc_txt="$oc_txt [Windows MSI for Group Policy]($oc_dl/win/$oc_win64_GPO_msi) |"
test -n "$oc_win32_GPO_msi" && oc_txt="$oc_txt [32 bit GPO]($oc_dl/win/$oc_win32_GPO_msi) |"
test -n "$oc_mac_pkg"       && oc_txt="$oc_txt [macOS]($oc_dl/mac/$oc_mac_pkg) |"
test -n "$oc_linux_dl"      && oc_txt="$oc_txt [Linux]($oc_dl/linux/$oc_linux_dl)"
test -n "$oc_txt"           && oc_txt="ownCloud client:$oc_txt"
oc_txt=$(echo "$oc_txt" | sed -e 's@|$@@')

tp_txt=""
test -n "$tp_win64_msi"     && tp_txt="$tp_txt [Windows MSI]($tp_dl/win/$tp_win64_msi) |"
test -n "$tp_win32_msi"     && tp_txt="$tp_txt [32 bit]($tp_dl/win/$tp_win32_msi) |"
test -n "$tp_win64_GPO_msi" && tp_txt="$tp_txt [Windows MSI for Group Policy]($tp_dl/win/$tp_win64_GPO_msi) |"
test -n "$tp_win32_GPO_msi" && tp_txt="$tp_txt [32 bit GPO]($tp_dl/win/$tp_win32_GPO_msi) |"
test -n "$tp_mac_pkg"       && tp_txt="$tp_txt [macOS]($tp_dl/mac/$tp_mac_pkg) |"
test -n "$tp_linux_dl"      && tp_txt="$tp_txt [Linux]($tp_dl/linux/$tp_linux_dl)"
test -n "$tp_txt"           && tp_txt="Testpilotcloud branded client:$tp_txt (independant config)"
tp_txt=$(echo "$tp_txt" | sed -e 's@|$@@')

test -n "$oc_source_asc" && oc_source_asc="$oc_dl/source/$oc_source_asc"
test -z "$oc_source_asc" && oc_source_asc="https://github.com/owncloud/client/issues/8109"	# FIXME!

src_txt=""
test -n "$oc_source"     && src_txt="$src_txt[Sources]($oc_dl/source/$oc_source)"
test -n "$oc_source_asc" && src_txt="$src_txt ([GPG Signature]($oc_source_asc))"
test -n "$src_txt"       && src_txt="Sources without optional dependencies:$src_txt"

cat <<EOF | tee $tag/github_release.md
$vers $(ucbeta $beta)

## Downloads
$oc_txt
$tp_txt
$src_txt


## ChangeLog
EOF


tmp=tmp-$$
mkdir $tmp
curl -k -L https://github.com/owncloud/client/archive/$tag.tar.gz | tar -C $tmp -zx --strip-components=1
if [ ! -d $tmp/changelog ]; then
  echo ""
  echo "https://github.com/owncloud/client/archive/$tag.tar.gz did not contain a changelog folder."
  echo "Please double check that '$tag' is a valid tag in https://github.com/owncloud/client/releases"
  exit 1
fi
# the single-quotes around 'EOF' preserve $ in the text.
cat <<'EOF' >> $tmp/changelog/MINI.tmpl
{{- range $index, $changes := . }}{{ with $changes -}}
Changelog for ownCloud Desktop Client {{ .Version }} ({{ .Date }})
=======
{{ range $entry := .Entries }}{{ with $entry }}
* {{ .Type }} - {{ .Title }}: [#{{ .PrimaryID }}]({{ .PrimaryURL }})
{{- end }}{{ end }}
{{ end }}{{ end -}}
EOF
rm -rf $tmp/changelog/*_*
mv $tmp/changelog/unreleased "$tmp/changelog/99.99.99_$(date +%Y-%m-%d)"
docker run -v $PWD/$tmp/changelog:/changelog toolhippie/calens:latest -t changelog/MINI.tmpl | sed -e "s/99\.99\.99/$vers $beta/" > $tag/CHANGELOG.md
rm -rf $tmp

# replace issue or pull urls with simple #NNN or #product/NNN notation.
sed -e 's@\[#[0-9]*\](https://github\.com/owncloud/\(client/\)\?@#@' -e 's@\([/#]\)\(pull\|issues\)/@\1@' -e 's/)$//' < $tag/CHANGELOG.md > $tag/CHANGELOG.txt
tee -a $tag/github_release.md < $tag/CHANGELOG.md
echo TODO: snippet for https://github.com/owncloud/enterprise/blob/master/client_update_checker/config/ownCloud.yml
