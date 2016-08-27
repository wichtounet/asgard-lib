node {
   stage 'git'
   checkout scm

   stage 'pre-analysis'
   sh 'cppcheck --xml-version=2 --enable=all --std=c++11 include/*.hpp 2> cppcheck_report.xml || true'
   sh 'sloccount --duplicates --wide --details include > sloccount.sc'
   sh 'cccc include/*.hpp || true'

   stage 'sonar'
   sh '/opt/sonar-runner/bin/sonar-runner'
}
