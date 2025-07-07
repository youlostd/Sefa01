# -*- coding: utf-8 -*-

import sys
sys.dont_write_bytecode = True
import script.export_proto as export_proto
import script.export_header as export_header

def main():
	export_proto.export()
	export_header.export()

	print("")
	print("==========================")
	print("|                        |")
	print("| Press any key to exit  |")
	print("|                        |")
	print("==========================")
	input()

if __name__ == '__main__':
    main()
