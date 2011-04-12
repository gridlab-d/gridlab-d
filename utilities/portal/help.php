<HTML>

  <HEAD>
    <TITLE>GridLAB-D&trade; Job Portal Help</TITLE>
  </HEAD>
  
  <BODY>
    <H1>Table of Contents</H1>
    <OL>
      <LI>Purpose</LI>
      <LI>Job Files</LI>
      <LI>Administrators</LI>
    </OL>
    For general help on GridLAB-D see <A HREF="http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Main_Page">GridLAB-D Help on SourceForge</A>.

  <H1>Purpose</H1>
    The GridLAB-D Job Portal is a queueing system for GridLAB-D jobs specifically designed for high-performance computers.
    The portal allows users to upload job files, which are then queued and run on the high-performance computer.
    <P/>
    A user may view the current state of the job by clicking on the job's status link.
    When the job is complete, the results are placed back in the job file, which can then be downloaded by the user by clicking on the job's status link.
    
  <H1>Job Files</H1>
    Job files contain GLM files that will be run by the portal.  
    Additional support files must also be included in the job file.
    <P/>
    The file <em>options.txt</em> may be included.  When included, the contents are used as the command line to <em>gridlabd</em>.
    <P/>
    Users should delete jobs when they are no longer needed or they have been downloaded.
  <H1>Administrator</H1>
    When logged in as <em>admin</em>, the user may start and stop the queue.  
    When stopped, users may upload jobs, but the jobs will not be run until the queue is started.
    <P/>
    The administrator may reset the job queue processor.  Doing so will stop the queue and clear the temporary files.  The administrator will have to manually restart the queue after resetting it.
    <P/>
    <P.>
    The administrator may view and clear the job control log using the "View Log" link and [Clear] button.
    <P/>
    The administrator may delete any job.
  </BODY>
  
</HTML>
