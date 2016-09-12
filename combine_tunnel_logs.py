#!/usr/bin/python
egress_packets = dict()
with open('/tmp/tunnelclient.egress.log') as client_egress_log:
    firstline = True
    for line in client_egress_log:
        if firstline:
            ( _, egress_initial_timestamp ) = line.split(':')
            firstline = False
        else:
            (timestamp, uid, size) = line.split('-')
            egress_packets[uid] = (timestamp, size)

unsorted_log = []
with open('/tmp/tunnelserver.ingress.log') as server_ingress_log:
    firstline = True
    for line in server_ingress_log:
        if firstline:
            ( _, ingress_initial_timestamp ) = line.split(':')
            firstline = False
        else:
            (timestamp, uid, size) = line.split('-')
            ingress_timestamp = int(timestamp) - (int(egress_initial_timestamp) - int(ingress_initial_timestamp))
            if uid in egress_packets:
                (egress_timestamp, egress_size) = egress_packets[uid]
                assert( size == egress_size )
                unsorted_log.append( egress_timestamp.strip() + ' + ' + str(int(size)) )
                unsorted_log.append( str(ingress_timestamp) + ' - ' + str(int(size)) + ' ' + str( ingress_timestamp - int(egress_timestamp) ) )

print("# base timestamp: 0" )
for line in sorted( unsorted_log ):
    print(line)
