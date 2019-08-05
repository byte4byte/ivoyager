#ifdef DEBUG_TASK_DATA_FUNCS
	
	{
		char buffer[100];
		LPARAM var;
		int idx;

		AddCustomTaskVar(loadUrlTask, 5, 1);
		AddCustomTaskVar(loadUrlTask, 0, 3);

		GetCustomTaskVar(loadUrlTask, 0, &var, NULL);
		if (var != 3) {
			wsprintf(buffer, "task data test 1 failed - expected %ld got %ld", 3, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}

		GetCustomTaskVar(loadUrlTask, 5, &var, NULL);
		if (var != 1) {
			wsprintf(buffer, "task data test 2 failed - expected %ld got %ld", 1, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}		

		GetCustomTaskVar(loadUrlTask, 1, &var, NULL);
		if (var != 0) {
			wsprintf(buffer, "task data test 3 failed - expected %ld got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}		

		idx = AddCustomTaskListData(loadUrlTask, 3, 1); // idx 0
		if (idx != 0) {
			wsprintf(buffer, "task data test 4 failed - expected %ld got %ld", 0, idx);
			MessageBox(NULL, buffer, "", MB_OK);
		}		
		idx = AddCustomTaskListData(loadUrlTask, 3, 2); // idx 1
		if (idx != 1) {
			wsprintf(buffer, "task data test 5 failed - expected %ld got %ld", 1, idx);
			MessageBox(NULL, buffer, "", MB_OK);
		}
		idx = AddCustomTaskListData(loadUrlTask, 3, 4); // idx 2
		if (idx != 2) {
			wsprintf(buffer, "task data test 5 failed - expected %ld got %ld", 2, idx);
			MessageBox(NULL, buffer, "", MB_OK);
		}


		var = GetNumCustomTaskListData(loadUrlTask, 3);
		if (var != 4) {
			wsprintf(buffer, "task data test 6 failed - expected %ld got %ld", 4, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}		
		//wsprintf(buffer, "num: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 3 [ 1, 2, 4 ]

		var = -1;
		GetCustomTaskListData(loadUrlTask, 3, 3, &var);
		if (var != 0) {
			wsprintf(buffer, "task data test 7 failed - expected %ld got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "idx3: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // -1

		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		if (var != 1) {
			wsprintf(buffer, "task data test 8 failed - expected %ld got %ld", 1, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "idx1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 1); // removed [ 1, 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		if (var != 4) {
			wsprintf(buffer, "task data test 9 failed - expected %ld got %ld", 4, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK);  // 4

		
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if (var != 1) {
			wsprintf(buffer, "task data test 10 failed - expected %ld got %ld", 1, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 0);  // removed [ 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if (var != 4) {
			wsprintf(buffer, "task data test 11 failed - expected %ld got %ld", 4, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 4

		RemoveCustomTaskListData(loadUrlTask, 3, 0);
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if (var != 0) {
			wsprintf(buffer, "task data test 12 failed - expected %ld got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // -1


	}
#endif