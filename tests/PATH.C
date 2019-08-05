#ifdef TEST_PATH_NORMAL
	{
		static LPSTR szPathTests[] = { "d:///////////////////index.html",
								 "d:/1\\/2/3\\\\\\.././././././././././////././././.\\..\\\\/\\hi//./././././index.html",
								 "/home/dir/"
							   };
		static LPSTR szPathResults[] = { "d:\\index.html",
								   "d:\\1\\hi\\index.html",
								   "C:\\home\\dir"
								 };									
		for (i = 0; i < sizeof(szPathTests)/sizeof(LPSTR); i++) {
			if (_fstricmp(NormalizePath(szPathTests[i]), szPathResults[i])) {
				MessageBox(NULL, szPathTests[i], "Path Normalize Failed", MB_OK);
			}
		}
	}
#endif