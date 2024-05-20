

public class ModbusException extends Exception {

    /**
     * Constructs a <code>ModbusException</code>
     * with the specified detail message.
     *
     * @param   s   the detail message.
     */
    public ModbusException(String str) {
        super(str);
    }

    /**
     * Constructs a <code>ModbusException</code>
     * with no detail message.
     */
    public ModbusException() {
        super();
    }
}
