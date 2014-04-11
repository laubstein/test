<?php

	printf("PW3270 PHP sample\n");

	$host = new tn3270("pw3270:a");

	$rc = $host->connect();
	print("connect() exits with rc=" . $rc . "\n");

	$rc = $host->waitforready(10);
	print("waitforready() exits with rc=" . $rc . "\n");

	$rc = $host->disconnect();
	print("disconnect() exits with rc=" . $rc . "\n");

?>
