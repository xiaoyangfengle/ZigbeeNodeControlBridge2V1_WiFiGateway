#!/usr/bin/python

import os, sys
import serial
import struct
import logging
import time
import string
import re

logger = logging.getLogger('RF TEST')


class myserial(serial.Serial):
    
    def __init__(self, *args, **kwargs):
        super(myserial, self).__init__(*args, **kwargs)
        self.haystack = ""
        
        self.logger = logging.getLogger("RF TEST:%s" % self.port)
        
        #self.logger.setLevel(logging.INFO)
    
    def read(self, *args, **kwargs):
        buf = super(myserial, self).read(*args, **kwargs)
        self.logger.debug("Rx: %s", buf)
        return buf
    
    def write(self, *args, **kwargs):
        self.logger.debug("Tx: %s", *args)
        res = super(myserial, self).write(*args, **kwargs)
        return res
        
    def expect(self, needle, failstr="", timeout=30):
        start = time.time()
        self.logger.debug("Waiting for string '%s' (fail '%s')", needle, failstr)
        
        while (time.time() < start + timeout):
            self.haystack = ''.join([self.haystack, self.read(100)])

            self.logger.debug("History now:\n%s", self.haystack)
            
            regex = re.compile(needle)
            match = re.search(regex, self.haystack)
            if match:
                self.logger.debug("needle found")
                
                line = self.haystack[self.haystack.find(match.group(0)):]
                #line = 
                line = line[:line.find("\n")]
                
                # Shift haystack along to the point after the needle
                self.haystack = self.haystack.split(match.group(0))[1]

                return line
            if failstr:
                if failstr in self.haystack:
                    raise ValueError("Error '%s' found in output." % failstr)

        raise ValueError("Expected data '%s' not received" % needle)
    
    def expectline(self, needle, failstr="", timeout=30):
        start = time.time()
        self.logger.debug("Waiting for line '%s' (fail '%s')", needle, failstr)
        
        while (time.time() < start + timeout):
            haystack = self.readline()

            if needle in haystack:
                self.logger.debug("needle found")
                return haystack
            if failstr:
                if failstr in self.haystack:
                    raise ValueError("Error '%s' found in output." % failstr)
    
    
class cPERtest(myserial):
    
    def __init__(self, port, baud):
        
        super(cPERtest, self).__init__(port, baudrate=baud, timeout=0.25)
        self.reset()
        
    def reset(self):
        self.write("\n\r")
        self.expect("\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*")
        self.expect("\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*")
        self.expect("\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*")

    def setchannel(self, channel):
        self.write("\n\r")
        self.expect("\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*")
        
        while True:
            line = self.expect("IoT Gateway Test: Channel ..       \*")
            currentchannel = int(line.split("IoT Gateway Test: Channel")[1].split("*")[0])
            
            self.logger.debug("Set to channel %d", currentchannel)
            
            if (channel == currentchannel):
                self.logger.info("Set to channel %d", currentchannel)
                return
            elif (channel > currentchannel):
                self.write("+")
                currentchannel = currentchannel + 1
            elif (channel < currentchannel):
                self.write("-")
                currentchannel = currentchannel - 1
        
        

    def master(self):
        self.write("M")
        
        result = {}
        
        self.expect("Locating")
        self.logger.info("Master Waiting")
        self.expect("Started")
        self.logger.info("Master Started")
        line = self.expect("PER")
        
        result["PER"] = float(line.split("PER:")[1].split("%")[0])
        line = self.expect("LQI")
        result["LQI"] = int(line.split("LQI:")[1].split("%")[0])
        return result
        
    def slave(self):
        self.write("S")
        self.expect("Started")
        self.logger.info("Slave Started")
        

def run(channel, d1, d2):
    logger.info("Test %s->%s on channel %d ...\n", d2.port, d1.port, channel)
    d1.setchannel(channel)
    d2.setchannel(channel)
    
    d1.slave()
    time.sleep(0.1)
    result = d2.master()
    d1.write("X")
    
    logger.info("Test %s->%s on channel %d PER=%f, LQI=%d\n", d2.port, d1.port, channel, result["PER"], result["LQI"])
    
    d1.reset()
    d2.reset()
    
    if (result["PER"] > 5.0):
        raise ValueError("PER Test channel %d failed: %f%%" % (channel, result["PER"]))
    
    if (result["LQI"] < 150):
        raise ValueError("LQI Test channel %d failed: %d" % (channel, result["LQI"]))
    
    
if __name__ == "__main__":
    from optparse import OptionParser
    parser = OptionParser()
    
    parser.add_option("-p", "--port", dest="ports",
                      help="Serial port to use", action="append", default=[])
    parser.add_option("-v", "--verbosity", dest="verbosity", type="string", default="info",
                    help="Verbosity of the server", metavar="VERBOSITY")
                    
    (options, args) = parser.parse_args()
                    
    if len(options.ports) < 2:
        print "Please specify 2 serial ports via -p or --port"
        sys.exit(0)

    # Set up verbosity
    options.verbosity = options.verbosity.lower()
    if (options.verbosity == "debug"):
        logging.basicConfig(format="%(asctime)-15s %(levelname)s:%(name)s:%(message)s")
        logging.getLogger().setLevel(logging.DEBUG)
    elif (options.verbosity == "info"):
        logging.basicConfig(format="%(name)s:%(message)s")
        logging.getLogger().setLevel(logging.INFO)
    elif (options.verbosity == "warning"):
        logging.getLogger().setLevel(logging.WARNING)
    elif (options.verbosity == "error"):
        logging.getLogger().setLevel(logging.ERROR)
    elif (options.verbosity == "critical"):
        logging.getLogger().setLevel(logging.CRITICAL)
    else:
        print "Unknown verbosity %s" % options.verbosity
        sys.exit(-1)

    ports = []

    for port in options.ports:
        try:
            p, b = port.split(":")
            b = int(b)
        except ValueError:
            p = port
            b = 1000000
        finally:
            ports.append((p, b))

    logger.info("Testing using ports %s@%d and %s@%d" % (ports[0][0], ports[0][1], ports[1][0], ports[1][1]))

    s1 = cPERtest(*ports[0])
    s2 = cPERtest(*ports[1])
    
    try:
        run(11, s1, s2)
        run(11, s2, s1)
        run(18, s1, s2)
        run(18, s2, s1)
        run(26, s1, s2)
        run(26, s2, s1)
        
    except ValueError, e:
        logger.info("FAILED:%s", str(e))
        exit(-1)

    logger.info("PASSED")
    exit(0)
                    
    
    
    