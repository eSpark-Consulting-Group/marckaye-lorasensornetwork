@echo Compiling
@javac -cp jcommon-1.0.17.jar;jfreechart-1.0.14.jar;azure-sdk.jar;lombok.jar;RXTXcomm.jar *.java
@jar cf lorasensor.jar *.class



