package com.bsuir.systemsoftwareforlans.command;

import com.bsuir.systemsoftwareforlans.connection.Connection;
import com.bsuir.systemsoftwareforlans.Controller;
import com.bsuir.systemsoftwareforlans.connection.IConnection;

import java.io.IOException;
import java.util.Arrays;

public class EchoCommand extends AbstractCommand {

    EchoCommand() {
        Arrays.stream(AvailableToken.values()).forEach(t -> availableTokens.put(t.getName(), t.getRegex()));
    }

    /**
     * Execute command.
     */
    @Override
    public void execute() {
        try {
            String content = getTokens().get(AvailableToken.CONTENT.getName());

            if (content != null) {
                executeEcho(content);
            }
        } catch (IOException e) {
            System.out.println("Error: " + e.getMessage());
        }
    }

    /**
     * Build command instance.
     *
     * @return instance
     */
    @Override
    public ICommand build() {
        return new EchoCommand();
    }

    private void executeEcho(String content) throws IOException {
        IConnection connection = Controller.getInstance().getConnection();
        connection.write(content);
    }

    private enum AvailableToken {
        CONTENT("content", null);

        private String name;
        private String regex;

        AvailableToken(String name, String regex) {
            this.name = name;
            this.regex = regex;
        }

        public String getName() {
            return name;
        }

        public String getRegex() {
            return regex;
        }
    }
}
