def countOccurrences(str, word):
      
    # split the string by spaces in a
    count=str.count(word)
    for i in range(0, count):
        ind=str.find(word)
        print(ind)
             
    return count 

def findAll(string,word):
    all_positions=[]
    next_pos=-1
    while True:
        next_pos=string.find(word,next_pos+1)
        if(next_pos<0):
            break
        all_positions.append(next_pos)
    return all_positions
def getIDList(string):
    lst=findAll(string,'nodeId:')
    ids=[]
    for i in range(0,len(lst)):
        ids.append(string[lst[i]+7:lst[i]+17])
    return ids

stringV=str('&{nodeId:3299856577,subs:[{nodeId:3299859357}]}\r\n')
print(stringV)
print(countOccurrences(stringV,'nodeId:'))

print(findAll(stringV,'nodeId:'))
getIDList(stringV)

oldIDs=['1','2','3']

liveIDs=['1','2']
for i in range(0,len(oldIDs)):
    if(oldIDs[i] not in liveIDs):
        print(oldIDs[i],'removed')
        break

oldIDs.remove('2')
print(oldIDs)
g="1,2,3,"
print(g[:-1])
import uuid,re
def getPIMAC():
    mac=''.join(re.findall('..', '%012x' % uuid.getnode()))
    return mac
print(getPIMAC())