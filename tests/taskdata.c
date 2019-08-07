#ifdef DEBUG_TASK_DATA_FUNCS
	
	{
		static char buffer[100];
		LPARAM var;
		int idx;

		AddCustomTaskVar(loadUrlTask, 5, 1);
		AddCustomTaskVar(loadUrlTask, 0, 3);

		GetCustomTaskVar(loadUrlTask, 0, &var, NULL);
		if ((int)var != 3) {
			wsprintf(buffer, "task data test 1 failed - expected %d got %ld", 3, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}

		GetCustomTaskVar(loadUrlTask, 5, &var, NULL);
		if ((int)var != 1) {
			wsprintf(buffer, "task data test 2 failed - expected %d got %ld", 1, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}		

		GetCustomTaskVar(loadUrlTask, 1, &var, NULL);
		if ((int)var != 0) {
			wsprintf(buffer, "task data test 3 failed - expected %d got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}		

		idx = AddCustomTaskListData(loadUrlTask, 3, 1); // idx 0
		if ((int)idx != 0) {
			wsprintf(buffer, "task data test 4 failed - expected %d got %d", 0, idx);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}		
		idx = AddCustomTaskListData(loadUrlTask, 3, 2); // idx 1
		if ((int)idx != 1) {
			wsprintf(buffer, "task data test 5 failed - expected %d got %d", 1, idx);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}
		idx = AddCustomTaskListData(loadUrlTask, 3, 4); // idx 2
		if ((int)idx != 2) {
			wsprintf(buffer, "task data test 5 failed - expected %d got %d", 2, idx);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}


		idx = GetNumCustomTaskListData(loadUrlTask, 3);
		if ((int)idx != 3) {
			wsprintf(buffer, "task data test 6 failed - expected %d got %d", 3, idx);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}		
		//wsprintf(buffer, "num: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 3 [ 1, 2, 4 ]

		var = -1;
		GetCustomTaskListData(loadUrlTask, 3, 3, &var);
		if ((int)var != 0) {
			wsprintf(buffer, "task data test 7 failed - expected %d got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "idx3: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // -1

		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		if ((int)var != 2) {
			wsprintf(buffer, "task data test 8 failed - expected %d got %ld", 2, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "idx1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 1); // removed [ 1, 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		if ((int)var != 4) {
			wsprintf(buffer, "task data test 9 failed - expected %d got %ld", 4, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK);  // 4

		
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if ((int)var != 1) {
			wsprintf(buffer, "task data test 10 failed - expected %d got %ld", 1, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 0);  // removed [ 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if ((int)var != 4) {
			wsprintf(buffer, "task data test 11 failed - expected %d got %ld", 4, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // 4

		RemoveCustomTaskListData(loadUrlTask, 3, 0);
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		if ((int)var != 0) {
			wsprintf(buffer, "task data test 12 failed - expected %d got %ld", 0, var);
			MessageBox(NULL, buffer, "", MB_OK);
			printTaskData(loadUrlTask);
		}				
		//wsprintf(buffer, "removed1: %d", var);
		//MessageBox(NULL, buffer, "", MB_OK); // -1

		RemoveAllCustomTaskData(loadUrlTask);

	}
#endif