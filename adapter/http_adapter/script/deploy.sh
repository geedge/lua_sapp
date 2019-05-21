SAPP_RUN=$1
if [ "${SAPP_RUN}" = "" ]; then
echo "USAGE: sh build_rpm.sh sapp_path" && exit 0
fi
mkdir -p $SAPP_RUN/plug/business/http_adapter
/bin/cp -f adapter/libhttp_adapter.so $SAPP_RUN/plug/business/http_adapter/libhttp_adapter.so
/bin/cp -f ../adapter/http_adapter/conf/http_adapter.inf $SAPP_RUN/plug/business/http_adapter/http_adapter.inf
/bin/cp -rf ../adapter/http_adapter/lua  $SAPP_RUN/plug
mkdir -p $SAPP_RUN/log/lua_sapp/http_adapter/lua/
cat $SAPP_RUN/plug/business/conflist_business.inf | grep "http_adapter.inf" >/dev/null 2>&1 && exit
echo ./plug/business/http_adapter/http_adapter.inf >> $SAPP_RUN/plug/business/conflist_business.inf