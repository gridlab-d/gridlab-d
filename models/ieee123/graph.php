<?php
	include("phpgraphlib.php");
	include("config/graph.php");

	$query = $_GET["query"] or die("missing query");
	$diff =  $_GET["diff"] or $diff = 0;	// compute diff of result
	$pos = $_GET["pos"] or $pos = 0;	// only use positive diffs
	$rev = $_GET["rev"] or $rev = 0;	// reverse the result
	$bar = $_GET["bar"] or $bar = 0;	// enable bar chart
	$title = $_GET["title"] or $title = "";
	$xinterval = $_GET["xinterval"] or $xinterval = 1;
	$ymin = $_GET["ymin"] or $ymin = 0;
	$ymax = $_GET["ymax"] or $ymax = 0;
	$xsize = $_GET["xsize"] or $xsize = 1800;
	$ysize = $_GET["ysize"] or $ysize = 1200;

	$link = mysql_connect($my_server, $my_user, $my_pwd)
		or die("mysql_connect('$my_server','$my_user','$my_pwd') failed: " . mysql_error());
     
	$data=array();
 	 
	$result = mysql_query($query)
		or die("mysql_query('$query') failed: " . mysql_error());
	$columns = 0;
	if ($result) 
	{
		$count = 0;
		while ($row = mysql_fetch_row($result)) 
		{
			$x = $row[0];
			if ( $columns < count($row) )
			{
				$columns = count($row);
			}
			for ( $n = 1 ; $n < count($row) ; $n++ )
			{
				$y = $row[$n];
				if ( $diff != 0 )
				{
      					if ( $count > 0 )
					{
						if ( $pos == 0 || ($yy > $y) )
							$data[$n][$x] = $yy - $y;
					}
					$yy = $y;
				}
				else
				{
					$data[$n][$x] = $y;
				}
			}
			$count++;
		}
	}
	$graph=new PHPGraphLib($xsize,$ysize); 
	$graph->setLine($bar==0);
	$graph->setBars($bar==1);
	for ( $n = 1 ; $n < $columns ; $n++ )
	{
		if ( $rev != 0 )
		{
			$data[$n] = array_reverse($data[$n],true);
		}	 
		$graph->addData($data[$n]);
	}
	if ( $bar==1 )
	{
		$graph->setGradient("lime", "green");
		$graph->setBarOutlineColor("black");
		$graph->setBackgroundColor("white");
	}
	else 
	{
		$graph->setLineColor("blue");
	}
	if ( strlen($title) > 0 )
		$graph->setTitle($title);
	$graph->setXValuesInterval(intval(preg_replace('[^0-9]','',$xinterval)));
	if ( $ymin < $ymax )
	{
		$graph->setRange($ymax,$ymin);
	}
	$graph->createGraph();
?>
