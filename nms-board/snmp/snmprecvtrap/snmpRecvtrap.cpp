#include <NmsSnmp.h>

void trap_callback( const CNmsSnmpPdu&       pdu,    // pdu passsed in
                    const Target&   target, // target passsed in
                    void * );                // optional callback data



int register_traps_to_receive( SnmpTrap& snmp );


int main( int argc, char **argv)
{
    int status;
    unsigned int trapPort=162;			// Default SNMP UDP Trap port to listen to is 162
    char *ptr;

    if ( argc > 1)                    // if more than 1 argument then we read the command line
    {
        if ( strstr( argv[1], "-p")!=0)  // parse for port
        {  
            ptr = argv[1]; ptr++; ptr++; 
            if( (*ptr=='\0') && (argc>2) )
            {
                ptr = argv[2];
            }
            trapPort = atoi( ptr);
            if(trapPort==0)
            {
                printf("\nInvalid trap port value specified with option '-p'\n");
                return 1;
            }
        }
        else
        {
            printf("Usage:\n");
            printf("snmpRecvTrap [options]\n");
            printf("options: -pPort, specify SNMP trap port to listen to (default is 162) \n");
            return 1;
        }
    }

    SnmpTrap trap( status );
    if( status != SNMP_CLASS_SUCCESS )
        goto exit;

    printf("Start to receive Traps on port %d\n", trapPort);
    status = trap.enable_traps( TRUE, trapPort );
    if( status != SNMP_CLASS_SUCCESS )
        goto exit;

    status = register_traps_to_receive( trap );
    if( status != SNMP_CLASS_SUCCESS )
        goto exit;


    trap.listen_traps();

exit:
    status ++;
return 1;
};


int register_traps_to_receive( SnmpTrap& trap )
{
    SnmpCallback callback;
    callback.m_pFunk = &trap_callback;
    return trap.notify_register( callback );
}

void trap_callback( const CNmsSnmpPdu&       pdu,    // pdu passsed in
                    const Target&   target, // target passsed in
                    void * )                 // optional callback data
{
int status;
        printf(" ------------ TRAP RECEIVED ---------------------\n");

        CNmsSnmpOid notify_oid;
        CNmsSnmpOid notify_enterprise_oid;

        pdu.get_notify_id( notify_oid );
        pdu.get_notify_enterprise( notify_enterprise_oid );

        printf("notify oid         : %s\n", notify_oid.get_printable());
        printf("notify enterprise  : %s\n", notify_enterprise_oid.get_printable());

        int nCount = pdu.get_vb_count();
        for( int i = 0; i < nCount; i ++ )
        {
            Vb vb;
            status = pdu.get_vb( vb, i );
            if( status == TRUE )
            {

                printf("\n");
                printf("%d\n", i);

                printf("oid                : %s\n", vb.get_printable_oid());
                printf("value              : %s\n", vb.get_printable_value());
                printf("\n");
            }
        }
        printf(" ------------ END TRAP --------------------------\n");
}
