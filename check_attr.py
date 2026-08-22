import urllib.request, re
html = urllib.request.urlopen('https://raw.githubusercontent.com/aosp-mirror/platform_frameworks_base/master/core/res/res/values/public.xml').read().decode('utf-8')
print('textViewStyle:', re.search(r'<public type="attr" name="textViewStyle" id="(0x[0-9a-fA-F]+)"', html).group(1))
print('buttonStyle:', re.search(r'<public type="attr" name="buttonStyle" id="(0x[0-9a-fA-F]+)"', html).group(1))
