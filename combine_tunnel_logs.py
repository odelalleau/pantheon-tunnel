#!/usr/bin/python
client_packets = dict()
with open('/tmp/tunnelclient.egress.log') as client_egress_log:
    firstline = True
    for line in client_egress_log:
        if firstline:
            ( _, _, client_initial_timestamp ) = line.split(':')
            firstline = False
        else:
            (timestamp, uid, size) = line.split('-')
            client_packets[int(uid)] = (int(timestamp), int(size))

unsorted_log = []
with open('/tmp/tunnelserver.ingress.log') as server_ingress_log:
    firstline = True
    for line in server_ingress_log:
        if firstline:
            ( _, _, server_initial_timestamp ) = line.split(':')
            firstline = False
        else:
            (timestamp, uid, size) = line.split('-')
            server_timestamp = int(timestamp) - (int(client_initial_timestamp) - int(server_initial_timestamp))
            if int(uid) in client_packets:
                (client_timestamp, client_size) = client_packets[int(uid)]
                if int(size) != client_size:
                    print("packet " + uid + " came into tunnel with size " + client_size + " but left with size " + size)
                    assert( False )
                unsorted_log.append( str(client_timestamp) + ' + ' + str(int(size)) )
                unsorted_log.append( str(server_timestamp) + ' # ' + str(int(size)) )
                unsorted_log.append( str(server_timestamp) + ' - ' + str(int(size)) + ' ' + str( server_timestamp - client_timestamp ) )

print("# base timestamp: 0" )
for line in sorted( unsorted_log, cmp=lambda x,y: cmp(int(x.split()[0]), int(y.split()[0])) ):
    print(line)
