# $AUTOMATE_LINUX_TRAP_GENERATOR_FILE
# return 0
# # This is an intentional error to test trap handling
# generateIntentionalError() {
#     return 127
# }
# generateIntentionalError
asdf1
( trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE background" ERR; asdf2 )
( trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE background" ERR; asdf3 ) &

