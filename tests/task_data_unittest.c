#ifdef DEBUG_TASK_DATA_FUNCS
	
	{
		char buffer[100];
		LPARAM var;
		int idx;

		AddCustomTaskVar(loadUrlTask, 5, 1);
		AddCustomTaskVar(loadUrlTask, 0, 3);

		GetCustomTaskVar(loadUrlTask, 0, &var, NULL);
		wsprintf(buffer, "%d", var);
		MessageBox(NULL, buffer, "", MB_OK);

		GetCustomTaskVar(loadUrlTask, 5, &var, NULL);
		wsprintf(buffer, "%d", var);
		MessageBox(NULL, buffer, "", MB_OK);

		GetCustomTaskVar(loadUrlTask, 1, &var, NULL);
		wsprintf(buffer, "%d", var);
		MessageBox(NULL, buffer, "", MB_OK);

		idx = AddCustomTaskListData(loadUrlTask, 3, 1); // idx 0
		wsprintf(buffer, "idx: %d", idx);
		MessageBox(NULL, buffer, "", MB_OK);
		idx = AddCustomTaskListData(loadUrlTask, 3, 2); // idx 1
		wsprintf(buffer, "idx: %d", idx);
		MessageBox(NULL, buffer, "", MB_OK);
		idx = AddCustomTaskListData(loadUrlTask, 3, 4); // idx 2
		wsprintf(buffer, "idx: %d", idx);
		MessageBox(NULL, buffer, "", MB_OK);


		var = GetNumCustomTaskListData(loadUrlTask, 3);
		wsprintf(buffer, "num: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // 3 [ 1, 2, 4 ]

		var = -1;
		GetCustomTaskListData(loadUrlTask, 3, 3, &var);
		wsprintf(buffer, "idx3: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // -1

		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		wsprintf(buffer, "idx1: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 1); // removed [ 1, 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 1, &var);
		wsprintf(buffer, "removed1: %d", var);
		MessageBox(NULL, buffer, "", MB_OK);  // 4

		
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		wsprintf(buffer, "removed1: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // 1

		RemoveCustomTaskListData(loadUrlTask, 3, 0);  // removed [ 4 ]
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		wsprintf(buffer, "removed1: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // 4

		RemoveCustomTaskListData(loadUrlTask, 3, 0);
		GetCustomTaskListData(loadUrlTask, 3, 0, &var);
		wsprintf(buffer, "removed1: %d", var);
		MessageBox(NULL, buffer, "", MB_OK); // -1


	}
#endif